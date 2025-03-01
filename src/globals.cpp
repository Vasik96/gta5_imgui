#include <d3d11.h>
#include <iostream>
#include <Windows.h>
#include <thread>
#include <chrono>

#include "globals.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "Logging.h"


bool globals::pForceHideWindow = true;


int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

namespace globals {
    void shutdown(HWND& window, IDXGISwapChain*& swap_chain, ID3D11DeviceContext*& device_context,
        ID3D11Device*& device, ID3D11RenderTargetView*& render_target_view) {

        isTimerRunning = false;
            
        Logging::CleanupTemporaryLogs();


        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (swap_chain) swap_chain->Release();
        if (device_context) device_context->Release();
        if (device) device->Release();
        if (render_target_view) render_target_view->Release();


        DestroyWindow(window);
    }
}





BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    char windowTitle[256];
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

    std::string title(windowTitle);
    if (title.find("FiveM") != std::string::npos || title.find("Grand Theft Auto V") != std::string::npos) {
        *(HWND*)lParam = hwnd;
        return FALSE; // Stop enumeration
    }

    return TRUE; // Continue enumeration
}

void FocusGame() {
    HWND hwnd = nullptr;
    EnumWindows(EnumWindowsProc, (LPARAM)&hwnd);

    if (hwnd) {
        std::cout << "Found FiveM window" << std::endl;

        // Ensure the game is not minimized
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }

        // Bring the window to the foreground
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);

        // Attach input threads if necessary
        DWORD fgThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        DWORD targetThreadId = GetWindowThreadProcessId(hwnd, NULL);

        if (fgThreadId != targetThreadId) {
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
        if (GetWindowRect(hwnd, &rect)) {
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

void globals::FocusGameNonBlocking() {
    std::thread focusThread(FocusGame);
    focusThread.detach();
}