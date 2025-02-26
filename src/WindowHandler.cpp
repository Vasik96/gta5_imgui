#include "WindowHandler.h"
#include <dwmapi.h>


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WNDCLASSEX wc = {};


LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
        return 0;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }


    //prevent ALT+F4, not intended to use alt+f4, use close button or "End" instead.
    if (message == WM_SYSCOMMAND) {
        if ((w_param & 0xFFF0) == SC_CLOSE) { // ALT+F4 triggers SC_CLOSE
            return 0; // Prevent closing
        }
        if ((w_param & 0xFFF0) == SC_KEYMENU) { // ALT+F4 triggers SC_CLOSE
            return 0; // Prevent closing
        }
    }


    return DefWindowProc(window, message, w_param, l_param);
}






bool InitializeWindow(HINSTANCE instance, HWND& window, int& screenWidth, int& screenHeight, int cmd_show) {
    
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = instance;
    wc.lpszClassName = L"External overlay class";

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    

    window = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"External Overlay",
        WS_POPUP,
        0, 
        0, 
        screenWidth,
        screenHeight,
        nullptr, 
        nullptr, 
        instance, 
        nullptr
    );

    if (!window) {
        return false;
    }

    SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    RECT client_area{};
    GetClientRect(window, &client_area);

    RECT window_area{};
    GetClientRect(window, &window_area);

    POINT diff{};
    ClientToScreen(window, &diff);

    const MARGINS margins{
        window_area.left + (diff.x - window_area.left),
        window_area.top + (diff.y - window_area.top),
        client_area.right,
        client_area.bottom
    };

    DwmExtendFrameIntoClientArea(window, &margins);

    ShowWindow(window, cmd_show);
    UpdateWindow(window);

    return true;
}

void CleanupWindow(HWND window) {
    if (window) {
        DestroyWindow(window);
    }
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}
