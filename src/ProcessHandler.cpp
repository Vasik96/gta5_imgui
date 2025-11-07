#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <string>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <shlobj.h>

// imgui
#include <imgui/imgui.h>

#include "ProcessHandler.h"
#include "Logging.h"
#include "globals.h"
#include "gui.h"


#pragma comment(lib, "psapi.lib")


typedef LONG(NTAPI* NtSuspendProcessFn)(IN HANDLE ProcessHandle);
typedef LONG(NTAPI* NtResumeProcessFn)(IN HANDLE ProcessHandle);



std::chrono::steady_clock::time_point sessionStart;

void InitializeSession() {
    static bool initialized = false;
    if (!initialized) {
        sessionStart = std::chrono::steady_clock::now();
        initialized = true;
    }
}


bool ProcessHandler::TerminateGTA5() {
    g_alert_notification.CreateNotification("GTA 5 has been closed");

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    bool processFound = false;
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);


    if (Process32First(snapshot, &processEntry)) {
        
        do {
            std::this_thread::yield();
            if (_wcsicmp(processEntry.szExeFile, globals::gtaProcess.c_str()) == 0 ||
                _wcsicmp(processEntry.szExeFile, L"FiveM.exe") == 0 ||
                _wcsnicmp(processEntry.szExeFile, L"FiveM_", 6) == 0) { // Check for processes starting with "FiveM_"
                HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, processEntry.th32ProcessID);
                if (process) {
                    TerminateProcess(process, 0);
                    CloseHandle(process);
                    processFound = true;
                }
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    
    CloseHandle(snapshot);
    return processFound;
}



std::string ProcessHandler::GetCurrentSession() {
    InitializeSession();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - sessionStart);

    std::ostringstream oss;
    oss << duration.count() / 3600 << "h "
        << (duration.count() % 3600) / 60 << "m "
        << duration.count() % 60 << "s";

    return oss.str();
}



bool ProcessHandler::IsProcessRunning(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    bool isRunning = false;

    if (Process32First(snapshot, &processEntry)) {
        do {
            std::this_thread::yield();
            if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                isRunning = true;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    return isRunning;
}

void ProcessHandler::ResetSession() {
    sessionStart = std::chrono::steady_clock::now();
}



void ShellExecuteFromExplorer(
    PCWSTR pszFile,
    PCWSTR pszParameters = nullptr,
    PCWSTR pszDirectory = nullptr,
    PCWSTR pszOperation = nullptr,
    int nShowCmd = SW_SHOWNORMAL)
{
    // Using ShellExecute with explorer to run the process as the current user
    HINSTANCE hInst = ShellExecute(NULL, L"open", L"explorer.exe", pszFile, pszDirectory, nShowCmd);
    if ((uintptr_t)hInst <= 32) {
        // Handle error
        Logging::Log("Failed to launch FiveM via explorer", 3);
        ImGui::Text("Failed to launch FiveM via explorer");
    }
}

void ProcessHandler::LaunchFiveM() {
    // Retrieve the current username from the environment variable
    wchar_t username[256];
    DWORD usernameLen = GetEnvironmentVariable(L"USERNAME", username, sizeof(username) / sizeof(wchar_t));

    if (usernameLen == 0) {
        Logging::Log("Failed to retrieve username", 3);
        ImGui::Text("Failed to retrieve username");
        return;
    }

    // Construct the FiveM path using the username
    std::wstring fiveMPath = L"C:\\Users\\" + std::wstring(username) + L"\\AppData\\Local\\FiveM\\FiveM.exe";

    Logging::Log("Launching FiveM...", 1);

    // Use ShellExecuteFromExplorer to launch FiveM without elevated privileges
    ShellExecuteFromExplorer(fiveMPath.c_str(), NULL, NULL, L"open", SW_SHOWNORMAL);
}


void ProcessHandler::LaunchGTA5(bool AntiCheatEnabled) {
    g_alert_notification.CreateNotification("Launching GTA 5...");

    // Define the path to Steam.exe (usually installed in the default directory)
    const std::wstring steamPath = L"C:\\Steam\\Steam.exe";

    // GTA5 App ID for Steam
    const std::wstring gta5AppID = L"3240220"; // This is the Steam App ID for GTA5 // enhanced: 3240220; legacy: 271590

    // Argument for launching GTA5 with or without BattleEye
    std::wstring arguments = L"-applaunch " + gta5AppID;
    if (!AntiCheatEnabled) {
        arguments += L" -nobattleye"; // Add the -nobattleye argument if AntiCheatEnabled is false
        Logging::Log("Launching GTA 5 with mods...", 1);
    }
    if (AntiCheatEnabled) {
        Logging::Log("Launching GTA 5...", 1);
    }

    // Create a new process to launch Steam with the correct arguments
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    if (!CreateProcess(steamPath.c_str(), (LPWSTR)arguments.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        // Handle error if the process creation fails
        Logging::Log("Launched GTA 5 successfully", 1);
        Logging::Log("Failed to launch GTA 5", 3);
        ImGui::Text("Failed to launch GTA 5");
    }
    else {
        // Wait for the process to be created successfully
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}



void ProcessHandler::DesyncFromGTAOnline() {
    // Check if GTA5.exe process is running
    if (!IsProcessRunning(globals::gtaProcess.c_str())) {
        return;
    }

    // Load ntdll.dll and resolve the function pointers for NtSuspendProcess and NtResumeProcess
    HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
    if (!ntdll) {
        ImGui::Text("Failed to load ntdll.dll");
        return;
    }


    auto NtSuspendProcess = (NtSuspendProcessFn)GetProcAddress(ntdll, "NtSuspendProcess");
    auto NtResumeProcess = (NtResumeProcessFn)GetProcAddress(ntdll, "NtResumeProcess");

    if (!NtSuspendProcess || !NtResumeProcess) {
        ImGui::Text("Failed to resolve NtSuspendProcess or NtResumeProcess");
        return;
    }

    // Take a snapshot of all processes
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        ImGui::Text("Failed to create process snapshot");
        return;
    }

    Logging::Log("Attempting to desync from GTA Online...", 1);


    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            std::this_thread::yield();
            if (_wcsicmp(processEntry.szExeFile, globals::gtaProcess.c_str()) == 0) {
                // Open the process with the necessary privileges to suspend and resume
                HANDLE process = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, processEntry.th32ProcessID);
                if (process) {
                    Logging::Log("Process suspended", 1);
                    // Suspend the process
                    NtSuspendProcess(process);

                    // Wait for 10 seconds to desync
                    std::this_thread::sleep_for(std::chrono::seconds(10));

                    Logging::Log("Process resumed", 1);

                    // Resume the process
                    NtResumeProcess(process);

                    Logging::Log("Desynced from GTA Online successfully", 1);
                    g_alert_notification.CreateNotification("Desynced from session");

                    CloseHandle(process);
                }
                else {
                    ImGui::Text("Failed to open GTA5.exe process");
                }
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
}

DWORD ProcessHandler::FindProcessIdByWindow(const std::wstring& windowTitle) {
    HWND hwnd = nullptr;

    if (windowTitle == L"FiveM") {


        struct EnumData {
            std::wstring search;
            HWND result = nullptr;
        } data{ windowTitle };

        EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
            auto& data = *reinterpret_cast<EnumData*>(lParam);

            wchar_t title[256];
            if (GetWindowTextW(hWnd, title, sizeof(title) / sizeof(wchar_t))) {
                std::wstring windowText(title);
                if (windowText != data.search && windowText.find(data.search) != std::wstring::npos) {
                    data.result = hWnd;
                    return FALSE; // stop enumeration
                }
            }
            return TRUE; // continue enumeration
            }, reinterpret_cast<LPARAM>(&data));

        hwnd = data.result;
        if (!hwnd) return 0;
    }
    else {
        hwnd = FindWindow(NULL, windowTitle.c_str());
        if (!hwnd) return 0;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    std::wcout << L"pid: " << pid << L"\n";
    return pid;
}

DWORD ProcessHandler::GetPIDByProcessName(const wchar_t* processName)
{
    DWORD PID = 0;
    HANDLE hProcessSnapshot;
    PROCESSENTRY32 PE32;

    // Take a snapshot of all processes in the system.
    hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (hProcessSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "<CreateToolhelp32Snapshot> Invalid handle";
        return 0;
    }

    // Set the size of the structure before using it.
    PE32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieves information about the first process and exit if unsuccessful
    if (!Process32First(hProcessSnapshot, &PE32))
    {
        std::cout << "<Process32First> Error " << GetLastError() << '\n';
        CloseHandle(hProcessSnapshot);
        return 0;
    }

    // Now walk the snapshot of processes,
    // and find the right process then get its PID
    do
    {
        // Returns 0 value indicates that both wchar_t* string are equal
        if (wcscmp(processName, PE32.szExeFile) == 0)
        {
            PID = PE32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnapshot, &PE32));

    CloseHandle(hProcessSnapshot);
    
    std::cout << "Final PID: " + std::to_string(PID) << std::endl;

    return PID;
}

std::string ProcessHandler::GetProcessNameFromPID(DWORD pid) {
    //std::cout << "Passed PID: " + std::to_string(pid) << std::endl;

    std::string ret;
    HANDLE handle = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE,
        pid
    );
    if (handle)
    {
        DWORD buffSize = 1024;
        CHAR buffer[1024];
        if (QueryFullProcessImageNameA(handle, 0, buffer, &buffSize))
        {
            ret = buffer;

            size_t pos = ret.find_last_of("\\/");
            if (pos != std::string::npos) {
                ret = ret.substr(pos + 1);
            }
        }
        else
        {
            printf("Error GetModuleBaseNameA : %lu", GetLastError());
        }
        CloseHandle(handle);
    }

    return ret;
}