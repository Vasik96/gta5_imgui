#define IMGUI_DEFINE_MATH_OPERATORS
//#define DISCORDPP_IMPLEMENTATION

#include "NetworkInfo.h"

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


//globals.h
ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_device_context = nullptr;
IDXGISwapChain* g_swap_chain = nullptr;
ID3D11RenderTargetView* g_render_target_view = nullptr;


bool globals::windowVisible = true;
bool globals::running = true;
bool globals::overlayToggled = true;

std::string globals::lastTimerResult = "N/A";


std::chrono::time_point<std::chrono::steady_clock> timerStart;


bool keybindThreadRunning = true;

bool wasF4KeyDown = false;


bool isTimerRunning = false;



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


HWND window = nullptr;



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


    if (GetAsyncKeyState(VK_END) & 0x8000) {
        globals::running = false;
    }


    if (visibilityStatus == VISIBLE) {
        ShowWindow(window, SW_SHOW);
        globals::windowVisible = true;
        SetWindowLongPtr(window, GWL_EXSTYLE,
            GetWindowLongPtr(window, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);

    }
}




void KeybindHandlerThread(HWND window)
{
    while (keybindThreadRunning)
    {
        HandleKeybinds(window);
        std::this_thread::yield();
    }
}

bool p_gta5NotRunning;
bool p_fivemNotRunning;

void DataThread()
{
    std::thread([]()
        {
        while (globals::running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            p_gta5NotRunning = !ProcessHandler::IsProcessRunning(globals::gtaProcess.c_str());
            p_fivemNotRunning = !ProcessHandler::IsProcessRunning(L"FiveM.exe");
            //Logging::CheckExternalLogFile();
            std::this_thread::sleep_for(std::chrono::seconds(1));
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

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{ 
    //AllocateConsole();

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



    InitializeImGui(window, g_device, g_device_context);
    DataThread();
    
    std::thread insertKeyThread(KeybindHandlerThread, window);
    insertKeyThread.detach();

    
    std::thread captureThread(NetworkInfo::startPacketCapture);
    captureThread.detach();

    ShowCursor(FALSE);

    std::thread discord_presence_thread(discordPresenceUpdater);
    discord_presence_thread.detach();

    // menu uptime timer
    globals::menu_uptime = std::chrono::steady_clock::now();

    
    StartEtwSession();
    globals::fps_process_thread = std::thread(OpenAndProcess);
    globals::fps_process_thread.detach();
    StartFpsCalculation();

    while (globals::running)
    {

        MSG msg;

        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                globals::running = false;
                globals::windowVisible = false;
            }
        }

        if (!globals::running)
        {
            break;
        }

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

        if (!globals::windowVisible)
        {
            continue;
        }



        if (visibilityStatus != HIDDEN || !globals::windowVisible && (!p_gta5NotRunning || !p_fivemNotRunning))
        {
            RenderImGui(
                p_gta5NotRunning,
                p_fivemNotRunning,
                window
            );



            constexpr float color[4] = { 0.f, 0.f, 0.f, 0.f };
            g_device_context->OMSetRenderTargets(1U, &g_render_target_view, nullptr);
            g_device_context->ClearRenderTargetView(g_render_target_view, color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); //this has to be here, otherwise the window will not appear

            g_swap_chain->Present(1U, 0U);
        }

        std::this_thread::yield();
    }
    StopEtwSession();

    Discord_Shutdown();

    globals::NO_SAVE__RemoveFirewallRule();

    

    keybindThreadRunning = false;
    if (insertKeyThread.joinable())
    {
        insertKeyThread.join();
    }

    
    if (captureThread.joinable())
    {
        captureThread.join();
    }

    
    //shutdown program
    globals::shutdown(window);

    CleanupDirectX(g_device, g_device_context, g_swap_chain, g_render_target_view);

    return 0;
}
