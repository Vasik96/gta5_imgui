#define IMGUI_DEFINE_MATH_OPERATORS
//#define DISCORDPP_IMPLEMENTATION

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
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <csignal>

#include "TicTacToe.h"
#include "ProcessHandler.h"
#include "FormattedInfo.h"
#include "Calculations.h"
#include "WindowHandler.h"
#include "DXHandler.h"
#include "gui.h"
#include "Logging.h"
#include "globals.h"
#include "ExternalLogger.h"
#include "KeyboardInput.h"

#include "imgui_styleGTA5/Include.hpp"

#include "discord/include/discord_register.h"
#include "discord/include/discord_rpc.h"

#include "FPSTracker.h"
#include "System.h"


extern void ShutdownHWiNFOMemory();


//globals.h
ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_device_context = nullptr;
IDXGISwapChain* g_swap_chain = nullptr;
ID3D11RenderTargetView* g_render_target_view = nullptr;


bool globals::windowVisible = true;
bool globals::running = true;
bool globals::overlayToggled = true;

bool keybindThreadRunning = true;
bool wasF4KeyDown = false;
bool isTimerRunning = false;
bool p_gta5NotRunning = false;
bool p_fivemNotRunning = false;

std::string globals::lastTimerResult = "N/A";

std::chrono::time_point<std::chrono::steady_clock> timerStart;

VisibilityStatus visibilityStatus = VISIBLE;

void AllocateConsole()
{
    // Allocate a new console
    AllocConsole();

    // Redirect standard output to the console
    FILE* fileStream;
    freopen_s(&fileStream, "CONOUT$", "w", stdout);
    freopen_s(&fileStream, "CONOUT$", "w", stderr);
    freopen_s(&fileStream, "CONIN$", "r", stdin);

    // Sync C++ streams with C streams
    std::ios::sync_with_stdio();
}

bool is_gta5_focused() {
    const int length = 256;
    wchar_t windowTitle[length] = { 0 };
    HWND hwnd = GetForegroundWindow();
    if (hwnd && GetWindowTextW(hwnd, windowTitle, length) > 0) {
        // Direct compare without std::wstring
        if (wcscmp(windowTitle, L"Grand Theft Auto V Enhanced") == 0 ||
            wcscmp(windowTitle, L"Grand Theft Auto V") == 0) {
            return true;
        }
    }
    return false;
}

bool is_gta5_currently_focused = false;

void HandleKeybinds(HWND window)
{
    if (KeyboardInput::insertKeyPressed) {
        if (!KeyboardInput::wasInsertKeyDown) {
            // hidden -> overlay (skipped if overlayToggled is false) -> visible
            if (visibilityStatus == VISIBLE) {
                if (!globals::overlayToggled) {
                    visibilityStatus = HIDDEN;
                }
                else {
                    visibilityStatus = OVERLAY;
                }
            }
            else if (visibilityStatus == HIDDEN) {
                globals::pForceHideWindow = true;
                visibilityStatus = VISIBLE;
            }
            else if (visibilityStatus == OVERLAY) {
                visibilityStatus = HIDDEN;
            }

            // Update visibility based on the current state
            if (visibilityStatus == VISIBLE) {
                ShowWindow(window, SW_SHOW); // Show main window
                globals::windowVisible = true;
                SetWindowLongPtr(window, GWL_EXSTYLE,
                    GetWindowLongPtr(window, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
                globals::pForceHideWindow = true;
            }
            else if (visibilityStatus == OVERLAY) {
                ShowWindow(window, SW_SHOW); // Show main window
                globals::windowVisible = true; // Keep overlay active
                SetWindowLongPtr(window, GWL_EXSTYLE,
                    GetWindowLongPtr(window, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
            }
            else if (visibilityStatus == HIDDEN) {
                ShowWindow(window, SW_HIDE); // Hide main window
                globals::windowVisible = false; // Disable overlay
            }
        }
        KeyboardInput::wasInsertKeyDown = true;
    }
    else {
        KeyboardInput::wasInsertKeyDown = false;
    }


    if (GetAsyncKeyState(VK_F5) & 0x8000) {
        if (!wasF4KeyDown) {
            if (globals::isTimerRunning) {
                // Stop the timer
                globals::lastTimerResult = FormattedInfo::GetFormattedTimer().c_str();
                globals::isTimerRunning = false;
                
            }
            else {
                // Start the timer
                globals::isTimerRunning = true;
                timerStart = std::chrono::steady_clock::now();
            }
        }
        wasF4KeyDown = true;
    }
    else {
        wasF4KeyDown = false;
    }


    if ((GetAsyncKeyState(VK_END) & 0x8000) && (GetAsyncKeyState(VK_LSHIFT) & 0x8000)) {
        globals::running = false;
    }


    if (visibilityStatus == VISIBLE) {
        ShowWindow(window, SW_SHOW);
        globals::windowVisible = true;
        SetWindowLongPtr(window, GWL_EXSTYLE,
            GetWindowLongPtr(window, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);

    }

    if (globals::real_alt_f4_enabled && is_gta5_currently_focused) {
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(VK_F4) & 0x8000)) {
            Logging::Log("terminating gta 5...", 1);
            ProcessHandler::TerminateGTA5();
        }
    }
}


void KeybindHandlerThread(HWND window)
{
    while (keybindThreadRunning)
    {
        HandleKeybinds(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool is_topmost(HWND hwnd)
{
    HWND hTopMost = GetTopWindow(nullptr);
    return (hTopMost == hwnd);
}


void DataThread()
{
    std::thread([]()
        {
        while (globals::running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            p_gta5NotRunning = !ProcessHandler::IsProcessRunning(globals::gtaProcess.c_str());
            p_fivemNotRunning = !ProcessHandler::IsProcessRunning(L"FiveM.exe");
            //Logging::CheckExternalLogFile();

            globals::is_gta5_running = !p_gta5NotRunning;
            globals::is_fivem_running = !p_fivemNotRunning;

            std::this_thread::sleep_for(std::chrono::seconds(1));

            is_gta5_currently_focused = is_gta5_focused();

            SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        }
    }).detach();
}


void discordPresenceUpdater() {
    globals::discord::Initialize();

    while (globals::running) {
        globals::discord::UpdatePresence();

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

std::thread globals::fps_process_thread;


static constexpr int TARGET_OVERLAY_FPS = 150;

void GuiRenderThread() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    const double target_frame_time = 1.0 / TARGET_OVERLAY_FPS;

    while (globals::running) {
        LARGE_INTEGER frame_start;
        QueryPerformanceCounter(&frame_start);

        if (globals::windowVisible &&
            (visibilityStatus != HIDDEN || (!p_gta5NotRunning || !p_fivemNotRunning)))
        {
            RenderImGui(p_gta5NotRunning, p_fivemNotRunning, window);

            static constexpr float color[4] = { 0.f, 0.f, 0.f, 0.f };
            g_device_context->OMSetRenderTargets(1U, &g_render_target_view, nullptr);
            g_device_context->ClearRenderTargetView(g_render_target_view, color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_swap_chain->Present(0U, 0U);
        }

        safety_checks::check_dx_validity();

        // Measure frame time
        LARGE_INTEGER frame_end;
        QueryPerformanceCounter(&frame_end);
        double elapsed = static_cast<double>(frame_end.QuadPart - frame_start.QuadPart) / frequency.QuadPart;

        double remaining = target_frame_time - elapsed;
        if (remaining > 0.0) {
            // Sleep for most of the remaining time, leave a tiny margin for precision
            if (remaining > 0.002)  // >2 ms
                std::this_thread::sleep_for(std::chrono::duration<double>(remaining - 0.001));

            // Final fine-tune busy wait for the last ~1 ms
            do {
                QueryPerformanceCounter(&frame_end);
                elapsed = static_cast<double>(frame_end.QuadPart - frame_start.QuadPart) / frequency.QuadPart;
            } while (elapsed < target_frame_time);
        }
    }
}



void SetCWDToExeFolder() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::wstring pathStr(exePath);
    size_t lastSlash = pathStr.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        std::wstring exeFolder = pathStr.substr(0, lastSlash);
        SetCurrentDirectoryW(exeFolder.c_str());
    }
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{ 
    timeBeginPeriod(1);
    //AllocateConsole();

    SetCWDToExeFolder();

    Logging::CleanupTemporaryLogs();

    Logging::Log("[WinMain] Initializing...", 1);

    if (!InitializeWindow(instance, window, screenWidth, screenHeight, cmd_show))
    {
        return 1;
    }

    if (!InitializeDirectX(window, g_device, g_device_context, g_swap_chain, g_render_target_view))
    {
        CleanupWindow(window);
        return 1;
    }
    else {
        QueryTotalVRAM(g_device);
    }

    InitializeImGui(window, g_device, g_device_context);
    DataThread();
    
    std::thread insertKeyThread(KeybindHandlerThread, window);
    insertKeyThread.detach();

    ShowCursor(FALSE);

    std::thread discord_presence_thread(discordPresenceUpdater);
    discord_presence_thread.detach();

    // menu uptime timer
    globals::menu_uptime = std::chrono::steady_clock::now();

    
    StartEtwSession();
    globals::fps_process_thread = std::thread(OpenAndProcess);
    globals::fps_process_thread.detach();
    StartPerformanceCalculations();


    std::thread gui_renderer_thread(GuiRenderThread);
    gui_renderer_thread.detach();

    g_alert_notification.CreateNotification("Overlay initialized");

    while (globals::running)
    {
        MSG msg;

        if (!PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            WaitMessage();
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);



        if (!globals::pForceHideWindow)
        {
            globals::pForceHideWindow = true;

            if (globals::overlayToggled)
            {
                visibilityStatus = OVERLAY;
            }
            else
            {
                visibilityStatus = HIDDEN;
                globals::windowVisible = false;
                ShowWindow(window, SW_HIDE);
            }
        }

        if (!p_gta5NotRunning || !p_fivemNotRunning)
        {
            ProcessHandler::GetCurrentSession();
        }
        
        //revert resolution after GTA 5 is closed
        if (!globals::is_gta5_running && !globals::is_fivem_running && has_resolution_changed_for_ssaa) {
            DEVMODE devMode = {};
            devMode.dmSize = sizeof(DEVMODE);
            if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &devMode)) {
                int width = devMode.dmPelsWidth;
                int height = devMode.dmPelsHeight;

                System::ChangeResolution(width, height);
                Logging::Log("Successfully reset resolution to the native value", 1);
            }
            else {
                System::ChangeResolution(1920, 1080);
                Logging::Log("Fallback to hardcoded resolution reset", 2);
            }

            has_resolution_changed_for_ssaa = false;
        }

        std::this_thread::yield();
        //std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    timeEndPeriod(1);

    globals::NO_SAVE__RemoveFirewallRule();

    StopEtwSession();
    Discord_Shutdown();

    keybindThreadRunning = false;
    if (insertKeyThread.joinable())
    {
        insertKeyThread.join();
    }

    //shutdown program
    globals::shutdown(window);
    CleanupDirectX(g_device, g_device_context, g_swap_chain, g_render_target_view);

    ShutdownHWiNFOMemory();

    return 0;
}
