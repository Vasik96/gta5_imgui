#define IMGUI_DEFINE_MATH_OPERATORS

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

#include "imgui_styleGTA5/Include.hpp"


bool globals::windowVisible = true;
bool globals::running = true;
bool globals::overlayToggled = true;

std::string globals::lastTimerResult = "N/A";


std::chrono::time_point<std::chrono::steady_clock> timerStart;


bool keybindThreadRunning = true;

static bool wasInsertKeyDown = false;
bool wasF4KeyDown = false;


bool isTimerRunning = false;



VisibilityStatus visibilityStatus = VISIBLE;


void AllocateConsole() {
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




HHOOK hKeyboardHook;
std::unordered_map<int, bool> activeKeys;
static std::unordered_map<int, bool> keyState;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (visibilityStatus == VISIBLE) {

            KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

            // Get the foreground window (the active window)
            HWND foregroundWindow = GetForegroundWindow();

            // Get the process ID of the active window
            DWORD processId;
            GetWindowThreadProcessId(foregroundWindow, &processId);

            // Get GTA 5's or FiveM's process ID, 
            // dont make it static -> if i would launch gta 5, then quit it and launch fivem, it would not detect the change.
            DWORD gtaProcessId = 0;
            if (gtaProcessId == 0) {
                HWND gtaWindow = FindWindow(NULL, L"Grand Theft Auto V");
                if (gtaWindow) {
                    GetWindowThreadProcessId(gtaWindow, &gtaProcessId);
                }
                else {
                    gtaWindow = FindWindow(NULL, L"FiveM");
                    if (gtaWindow) {
                        GetWindowThreadProcessId(gtaWindow, &gtaProcessId);
                    }
                }
            }

            // Define keys to block for GTA/FiveM when the menu is open
            bool isBlockedKey = (
                pKeyboard->vkCode == VK_RETURN ||
                pKeyboard->vkCode == VK_ESCAPE ||
                pKeyboard->vkCode == VK_UP ||
                pKeyboard->vkCode == VK_DOWN ||
                pKeyboard->vkCode == VK_LEFT ||
                pKeyboard->vkCode == VK_RIGHT ||
                pKeyboard->vkCode == 0x54 ||  // T key
                pKeyboard->vkCode == 0x52 ||  // R key
                pKeyboard->vkCode == 0x59 ||  // Y key
                pKeyboard->vkCode == VK_LBUTTON || // Left mouse button
                pKeyboard->vkCode == VK_RBUTTON  // Right mouse button
                );


            // We simulate the key press for ImGui by posting the WM_KEYDOWN/WM_KEYUP message.
            WPARAM key = pKeyboard->vkCode;
            LPARAM lParam = pKeyboard->scanCode | (pKeyboard->flags << 16);  // Pack the scan code and flags into lParam



            ImGuiIO& io = ImGui::GetIO();


            if (wParam == WM_KEYDOWN) {
                if (!keyState[key]) {
                    BYTE keyboardState[256];
                    GetKeyboardState(keyboardState); //get keyboard state.
                    WCHAR buffer[2];
                    int result = ToUnicodeEx(
                        pKeyboard->vkCode,
                        pKeyboard->scanCode,
                        keyboardState,
                        buffer,
                        2,
                        1,
                        GetKeyboardLayout(0)
                    );
                    if (result > 0) {
                        io.AddInputCharacter(buffer[0]);
                    }
                    keyState[key] = true;
                }
            }
            else if (wParam == WM_KEYUP) {
                keyState[key] = false;
            }

            // If menu is open and we're inside GTA 5 or FiveM, block only specific keys
            if (processId == gtaProcessId && isBlockedKey) {
                std::cout << "Blocking key for GTA/FiveM: " << pKeyboard->vkCode << std::endl;
                return 1;  // Block this key for GTA 5 or FiveM
            }

            // Otherwise, let the key go through (so it can interact with other apps, like the menu)
            SendMessage(window, wParam, pKeyboard->vkCode, lParam);
        }
    }

    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}


void InstallKeyboardHook() {
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!hKeyboardHook) {
        std::cerr << "Failed to install keyboard hook!" << std::endl;
    }
}

void UninstallKeyboardHook() {
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
    }
}





void HandleGameWindows() { // this is too annoying, it flickers the game window, so it wont look good, but it works. but dont use it

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
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        SendInput(1, &input, sizeof(INPUT));

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        ZeroMemory(&input, sizeof(INPUT));
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

        SendInput(1, &input, sizeof(INPUT));




        // Bring the ImGui window to the foreground
        //SetForegroundWindow(window);
        // Set focus to the ImGui window explicitly (if needed)
        SetFocus(window);

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        //simulate click to gain focus
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        SendInput(1, &input, sizeof(INPUT));

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        ZeroMemory(&input, sizeof(INPUT));
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

        SendInput(1, &input, sizeof(INPUT));
    }
}






void HandleKeybinds(HWND window) {
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

                //FocusImGui();


                
            }

            else if (visibilityStatus == OVERLAY) {
                ShowWindow(window, SW_SHOW); // Hide main window
                globals::windowVisible = true; // Keep overlay active
                SetWindowLongPtr(window, GWL_EXSTYLE,
                    GetWindowLongPtr(window, GWL_EXSTYLE) | WS_EX_TRANSPARENT);



                //HandleGameWindows();
               
                //globals::FocusGameNonBlocking();



            }
            else if (visibilityStatus == HIDDEN) {
                ShowWindow(window, SW_HIDE); // Hide main window
                globals::windowVisible = false; // Disable overlay

                //globals::FocusGameNonBlocking();

            }
        }
        wasInsertKeyDown = true;
    }
    else {
        wasInsertKeyDown = false;
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



}




void KeybindHandlerThread(HWND window) {
    while (keybindThreadRunning) {
        HandleKeybinds(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}


INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) { 
    //AllocateConsole();

    InstallKeyboardHook();

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

    
    
    std::thread insertKeyThread(KeybindHandlerThread, window);
    insertKeyThread.detach();

    ShowCursor(FALSE);

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

        if (!globals::pForceHideWindow) {
            globals::pForceHideWindow = true;

            if (globals::overlayToggled) {
                visibilityStatus = OVERLAY;
            }
            else {
                visibilityStatus = HIDDEN;
                globals::windowVisible = false;
                ShowWindow(window, SW_HIDE);
            }
        }



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
                window
            );



            constexpr float color[4] = { 0.f, 0.f, 0.f, 0.f };
            device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
            if (render_target_view != nullptr) {
                device_context->ClearRenderTargetView(render_target_view, color);
            }

            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); //this has to be here, otherwise the window will not appear

            swap_chain->Present(1U, 0U);



        }


        Logging::CheckExternalLogFile();

        std::this_thread::yield();
    }


    UninstallKeyboardHook();

    keybindThreadRunning = false;
    if (insertKeyThread.joinable()) {
        insertKeyThread.join();
    }

    //shutdown program
    globals::shutdown(
        window,
        swap_chain,
        device_context,
        device,
        render_target_view
    );


    return 0;
}
