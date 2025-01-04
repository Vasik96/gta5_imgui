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

bool globals::windowVisible = true;
bool globals::running = true;
bool globals::overlayToggled = true;


static bool wasInsertKeyDown = false;


VisibilityStatus visibilityStatus = VISIBLE;

int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

HWND window = nullptr;



void HandleGameWindows() {

    SetFocus(NULL);

    // Focus on "FiveM" window if process is running
    if (ProcessHandler::IsProcessRunning(L"FiveM.exe")) {
        HWND fiveMWindow = FindWindow(NULL, L"FiveM");

        if (fiveMWindow) {
          //  Logging::Log("FiveM window found", 1);
            SetForegroundWindow(fiveMWindow);
        }
    }
    // Optionally focus on "GTA5" window if process is running
    else if (ProcessHandler::IsProcessRunning(L"GTA5.exe")) {
        HWND gtaWindow = FindWindow(NULL, L"Grand Theft Auto V");

        if (gtaWindow) {
           // Logging::Log("GTA 5 window found", 1);
            SetForegroundWindow(gtaWindow);
        }
    }
}


void FocusImGui() {
    if (window) {
        // Bring the ImGui window to the foreground
        SetForegroundWindow(window);
        // Set focus to the ImGui window explicitly (if needed)
        SetFocus(window);
    }
}



void HandleInsertKey(HWND window) {
    if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
        if (!wasInsertKeyDown) {



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


                FocusImGui();


                
            }

            else if (visibilityStatus == OVERLAY) {
                ShowWindow(window, SW_SHOW); // Hide main window
                globals::windowVisible = true; // Keep overlay active
                SetWindowLongPtr(window, GWL_EXSTYLE,
                    GetWindowLongPtr(window, GWL_EXSTYLE) | WS_EX_TRANSPARENT);



                HandleGameWindows();
               



            }
            else if (visibilityStatus == HIDDEN) {
                ShowWindow(window, SW_HIDE); // Hide main window
                globals::windowVisible = false; // Disable overlay



            }
        }
        wasInsertKeyDown = true;
    }
    else {
        wasInsertKeyDown = false;
    }
}



INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) { 
    Logging::CleanupTemporaryLogs();

    Logging::Log("[WinMain] Initializing...", 1);

    if (!InitializeWindow(instance, window, screenWidth, screenHeight, cmd_show)) {
        return 1;
    }

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* device_context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    ID3D11RenderTargetView* render_target_view = nullptr;


    if (!InitializeDirectX(window, device, device_context, swap_chain, render_target_view)) {
        CleanupWindow(window);
        return 1;
    }


    InitializeImGui(window, device, device_context);
    
    

    
    

    while (globals::running) {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                globals::running = false;
                globals::windowVisible = false;
            }
        }

        if (!globals::running) {
            break;
        }


        HandleInsertKey(window);




        if (GetAsyncKeyState(VK_END) & 0x8000) {
            globals::running = false;
        }


        bool p_gta5NotRunning = !ProcessHandler::IsProcessRunning(L"GTA5.exe");
        bool p_fivemNotRunning = !ProcessHandler::IsProcessRunning(L"FiveM.exe");


        if (!p_gta5NotRunning || !p_fivemNotRunning) {
            ProcessHandler::GetCurrentSession();
        }

        if (!globals::windowVisible) {
            continue;
        }



        if (visibilityStatus != HIDDEN || !globals::windowVisible) {

            RenderImGui(
                p_gta5NotRunning,
                p_fivemNotRunning,
                window,
                device_context,
                render_target_view,
                swap_chain
            );

        }


        Logging::CheckExternalLogFile();

        
    }

    Logging::CleanupTemporaryLogs();


    ShutdownImGui();

    if (swap_chain) swap_chain->Release();
    if (device_context) device_context->Release();
    if (device) device->Release();
    if (render_target_view) render_target_view->Release();

    DestroyWindow(window);

    return 0;
}
