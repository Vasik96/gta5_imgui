#include <d3d11.h>
#include <iostream>
#include <Windows.h>
#include <thread>
#include <chrono>
#include <cstdlib>

#include "globals.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "Logging.h"
#include "ProcessHandler.h"

#include "discord/include/discord_register.h"
#include "discord/include/discord_rpc.h"

const std::wstring globals::gtaProcess = L"GTA5_Enhanced.exe";
std::chrono::time_point<std::chrono::steady_clock> globals::menu_uptime;

bool globals::pForceHideWindow = true;
bool globals::real_alt_f4_enabled = false;



int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

namespace globals
{
    bool is_gta5_running = false;
    bool is_fivem_running = false;

    namespace discord
    {
        const char* APPLICATION_ID = "1363270070301884536";
        static const time_t startTime = time(0);

        void UpdatePresence()
        {
            std::cout << "[discord] updating presence\n";


            DiscordRichPresence discordPresence;
            memset(&discordPresence, 0, sizeof(discordPresence));

            if (visibilityStatus == VISIBLE)
                discordPresence.details = "in menu";
            else if (visibilityStatus == OVERLAY)
                discordPresence.details = "overlay";
            else
                discordPresence.details = "menu hidden";

            std::string stateText = "Idle";
            if (ProcessHandler::IsProcessRunning(globals::gtaProcess.c_str()))
            {
                stateText = "Playing GTA 5";
            }
            else if (ProcessHandler::IsProcessRunning(L"FiveM.exe"))
            {
                stateText = "Playing FiveM";
            }

            discordPresence.state = stateText.c_str();
            discordPresence.startTimestamp = startTime;
            Discord_UpdatePresence(&discordPresence);
        }

        void Initialize()
        {
            DiscordEventHandlers handlers;
            memset(&handlers, 0, sizeof(handlers));
            Discord_Initialize(APPLICATION_ID, &handlers, TRUE, nullptr);
        }
    }



    void ExecuteCommandAsync(const char* command)
    {
        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION pi;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE; // Hide command window

        if (CreateProcessA(nullptr, (LPSTR)command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }

    void NO_SAVE__AddFirewallRule()
    {
        ExecuteCommandAsync(R"(cmd /c netsh advfirewall firewall add rule name="gta5_nosave" dir=out action=block remoteip=192.81.241.171)");
    }

    void NO_SAVE__RemoveFirewallRule()
    {
        ExecuteCommandAsync(R"(cmd /c netsh advfirewall firewall delete rule name="gta5_nosave")");
    }


    void shutdown(HWND& window)
    {

        isTimerRunning = false;
            
        Logging::CleanupTemporaryLogs();


        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();


        DestroyWindow(window);
    }
}





BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    char windowTitle[256];
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

    std::string title(windowTitle);
    if (title.find("FiveM") != std::string::npos || title.find("Grand Theft Auto V") != std::string::npos)
    {
        *(HWND*)lParam = hwnd;
        return FALSE; // Stop enumeration
    }

    return TRUE; // Continue enumeration
}

void FocusGame()
{
    HWND hwnd = nullptr;
    EnumWindows(EnumWindowsProc, (LPARAM)&hwnd);

    if (hwnd)
    {
        std::cout << "Found FiveM window" << std::endl;

        // Ensure the game is not minimized
        if (IsIconic(hwnd))
        {
            ShowWindow(hwnd, SW_RESTORE);
        }

        // Bring the window to the foreground
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);

        // Attach input threads if necessary
        DWORD fgThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        DWORD targetThreadId = GetWindowThreadProcessId(hwnd, NULL);

        if (fgThreadId != targetThreadId)
        {
            AttachThreadInput(fgThreadId, targetThreadId, TRUE);
            SetForegroundWindow(hwnd);
            AttachThreadInput(fgThreadId, targetThreadId, FALSE);
        }

        // Prevent Windows from treating this as a background process
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);

        // Hide cursor to prevent Windows from keeping it visible
        ShowCursor(FALSE);
        
        // Force Raw Input Capture
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01; // Generic desktop controls
        rid.usUsage = 0x02;     // Mouse
        rid.dwFlags = RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE; // Disable legacy input & capture mouse
        rid.hwndTarget = hwnd;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));

        
        // Clip cursor inside game window (forces input capture)
        RECT rect;
        if (GetWindowRect(hwnd, &rect))
        {
            ClipCursor(&rect);
        }

        // Move the cursor inside the game window to trigger capture
        SetCursorPos(rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2);


        // Force the game to recognize mouse input
        SetCapture(hwnd);
        ReleaseCapture();
        SetCapture(hwnd);

        // Force input activation
        SendMessage(hwnd, WM_ACTIVATE, WA_ACTIVE, 0);
        SendMessage(hwnd, WM_SETFOCUS, 0, 0);

        // Toggle cursor visibility to force it to update
        ShowCursor(TRUE);
        ShowCursor(FALSE);

        // Move the cursor again to ensure it's within the game window
        SetCursorPos(rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2);

        SetCursor(nullptr);
        


        RECT rect2;
        rect2.left = GetSystemMetrics(SM_CXSCREEN) - 1; // Rightmost X
        rect2.top = 0; // Top Y position
        rect2.right = rect2.left + 1; // Lock to 1-pixel width
        rect2.bottom = rect2.top + 1; // Lock to 1-pixel height
        ClipCursor(&rect2);

        Sleep(1000);
        ClipCursor(nullptr);
    }
}