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


// imgui
#include <imgui/imgui.h>

#include "ProcessHandler.h"
#include "Logging.h"


typedef LONG(NTAPI* NtSuspendProcessFn)(IN HANDLE ProcessHandle);
typedef LONG(NTAPI* NtResumeProcessFn)(IN HANDLE ProcessHandle);



namespace {
    std::chrono::steady_clock::time_point sessionStart;

    void InitializeSession() {
        static bool initialized = false;
        if (!initialized) {
            sessionStart = std::chrono::steady_clock::now();
            initialized = true;
        }
    }
}




bool ProcessHandler::TerminateGTA5() {

    

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    bool processFound = false;
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);


    if (Process32First(snapshot, &processEntry)) {
        
        do {
            if (_wcsicmp(processEntry.szExeFile, L"GTA5.exe") == 0 ||
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


void ProcessHandler::LaunchFiveM() {
    // Define the path to FiveM.exe
    const std::wstring fiveMPath = L"C:\\FiveM\\FiveM.exe";

    Logging::Log("Launching FiveM...", 1);

    // Create a new process to launch FiveM
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    if (!CreateProcess(fiveMPath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        // Handle error if the process creation fails
        Logging::Log("Failed to launch FiveM", 3);
        ImGui::Text("Failed to launch FiveM");
    }
    else {
        // Wait for the process to be created successfully
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void ProcessHandler::LaunchGTA5(bool AntiCheatEnabled, bool intoOnline) {
    // Define the path to Steam.exe (usually installed in the default directory)
    const std::wstring steamPath = L"C:\\Steam\\Steam.exe";

    // GTA5 App ID for Steam
    const std::wstring gta5AppID = L"271590"; // This is the Steam App ID for GTA5

    // Argument for launching GTA5 with or without BattleEye
    std::wstring arguments = L"-applaunch " + gta5AppID;
    if (!AntiCheatEnabled) {
        arguments += L" -nobattleye"; // Add the -nobattleye argument if AntiCheatEnabled is false
        Logging::Log("Launching GTA 5 with mods...", 1);
    }
    if (intoOnline && AntiCheatEnabled) {
        arguments += L" -StraightIntoFreemode"; //add argument to go into online without having to click the button
        Logging::Log("Launching GTA Online...", 1);
    }
    if (!intoOnline && AntiCheatEnabled) {
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
    if (!IsProcessRunning(L"GTA5.exe")) {
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
            if (_wcsicmp(processEntry.szExeFile, L"GTA5.exe") == 0) {
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