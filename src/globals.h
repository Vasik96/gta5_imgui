#pragma once
#include <d3d11.h>
#include <iostream>
#include <Windows.h>

#include <chrono>
#include "FPSTracker.h"
#include <thread>


extern int screenWidth;
extern int screenHeight;

namespace globals
{
    extern std::chrono::time_point<std::chrono::steady_clock> menu_uptime;


    namespace discord
    {
        void UpdatePresence();
        void Initialize();
    }


    extern std::string lastTimerResult;

    extern bool isTimerRunning;

	extern bool windowVisible;
	extern bool running;

    extern bool overlayToggled;

    extern bool pForceHideWindow;

    extern const std::wstring gtaProcess;


    void NO_SAVE__AddFirewallRule();
    void NO_SAVE__RemoveFirewallRule();

    void shutdown(HWND& window);

    extern std::thread fps_process_thread;

}

enum VisibilityStatus
{
    VISIBLE,
    OVERLAY,
    HIDDEN
};


extern std::chrono::time_point<std::chrono::steady_clock> timerStart;




extern VisibilityStatus visibilityStatus;


extern ID3D11Device* g_device;
extern ID3D11DeviceContext* g_device_context;
extern IDXGISwapChain* g_swap_chain;
extern ID3D11RenderTargetView* g_render_target_view;