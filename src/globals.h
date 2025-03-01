#pragma once
#include <d3d11.h>
#include <iostream>
#include <Windows.h>

#include <chrono>


extern int screenWidth;
extern int screenHeight;

namespace globals {

    extern std::string lastTimerResult;

    extern bool isTimerRunning;

	extern bool windowVisible;
	extern bool running;

    extern bool overlayToggled;

    extern bool pForceHideWindow;

    void shutdown(HWND& window, IDXGISwapChain*& swap_chain, ID3D11DeviceContext*& device_context,
        ID3D11Device*& device, ID3D11RenderTargetView*& render_target_view);


    void FocusGameNonBlocking();

}

enum VisibilityStatus {
    VISIBLE,
    OVERLAY,
    HIDDEN
};


extern std::chrono::time_point<std::chrono::steady_clock> timerStart;




extern VisibilityStatus visibilityStatus;