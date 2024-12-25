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


// imgui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

#include "TicTacToe.h"
#include "ProcessHandler.h"






extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
        return 0L;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0L;
    }

    return DefWindowProc(window, message, w_param, l_param);
}

int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);









///non backend related stuff...

//gets the system time in a formatted way (example: 15:30)
std::string GetFormattedTime() {
    // Get current system time
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    // Convert to local time safely using localtime_s
    std::tm localTime;
    localtime_s(&localTime, &nowTime);  // thread-safe version of localtime

    // Format time as HH:MM
    std::ostringstream formattedTime;
    formattedTime << std::setw(2) << std::setfill('0') << localTime.tm_hour << ":"
        << std::setw(2) << std::setfill('0') << localTime.tm_min;

    return formattedTime.str();
}













INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {
    WNDCLASSEX wc = { };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_procedure;
    wc.hInstance = instance;
    wc.lpszClassName = L"External overlay class";

    RegisterClassExW(&wc);

    const HWND window = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        L"External Overlay",
        WS_POPUP,
        0,
        0,
        screenWidth,
        screenHeight,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    {
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
    }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.RefreshRate.Numerator = 60U;
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = window;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL levels[2]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    ID3D11Device* device{ nullptr };
    ID3D11DeviceContext* device_context{ nullptr };
    IDXGISwapChain* swap_chain{ nullptr };
    ID3D11RenderTargetView* render_target_view{ nullptr };
    D3D_FEATURE_LEVEL level{};

    // Create device & other stuff
    D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0U,
        levels,
        2U,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device,
        &level,
        &device_context
    );

    ID3D11Texture2D* back_buffer{ nullptr };
    swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

    if (back_buffer) {
        device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
        back_buffer->Release();
    }
    else {
        return 1;
    }

    ShowWindow(window, cmd_show);
    UpdateWindow(window);

    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO(); 
    //(void)io;



    ImGui::StyleColorsClassic();

    ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.6f;

    //load font
    //io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 12.f);

    // Custom button style
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 3.0f;
    style.WindowRounding = 3.0f;

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);

    bool windowVisible = true;
    bool running = true;
    bool showWindow = true;
    bool gta5NotRunning = false;
    bool fivemNotRunning = false;
    bool showImGuiDebugWindow = false;
    float gta5_fps = 0.0f;
    int clickedTimes = 0;
    int currentSelection = 0;

    //handle window toggle (show/hide)
    static bool wasInsertKeyDown = false;



    TicTacToe::ResetTicTacToe();

    while (running) {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                running = false;
            }
        }

        if (!running) {
            break;
        }

        // Check for a key press to toggle visibility (e.g., F1)
        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (!wasInsertKeyDown) { // Only toggle on key down transition
                windowVisible = !windowVisible;

                if (windowVisible) {
                    ShowWindow(window, SW_SHOW);
                }
                else {
                    ShowWindow(window, SW_HIDE);
                    SetCursor(nullptr);
                }
            }
            wasInsertKeyDown = true; // Mark the key as being held
        }
        else {
            wasInsertKeyDown = false; // Reset when the key is released
        }

        if (GetAsyncKeyState(VK_END) & 0x8000) {
            running = false;
        }

        if (!windowVisible) {
            continue;
        }

        // Process ImGui rendering only when the window is visible
        if (windowVisible) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();

            ImGui::NewFrame();

            /*imgui windows:
            ImGui::Begin("window 1");
            ImGui::Text("example");

            ImGui::End();*/


            // imgui window
            ImGui::Begin("GTA 5 ImGui [External]", &showWindow);

            ImGui::Text("Time: %s", GetFormattedTime().c_str());


            bool p_gta5NotRunning = !ProcessHandler::IsProcessRunning(L"GTA5.exe");
            bool p_fivemNotRunning = !ProcessHandler::IsProcessRunning(L"FiveM.exe");

            if (!p_gta5NotRunning || !p_fivemNotRunning) {
                ImGui::Text("Current session: %s", ProcessHandler::GetCurrentSession().c_str());
            }
            else {
                ProcessHandler::ResetSession();
                ImGui::Text("No active GTA 5 or FiveM session");
            }


            if (p_gta5NotRunning && p_fivemNotRunning) {

                ImGui::SetNextItemWidth(128.0f);

                // Create the combo box with three selections
                const char* comboItems[] = { "GTA 5", "GTA 5 (Modded)", "FiveM" };
                if (ImGui::BeginCombo("", comboItems[currentSelection])) {
                    for (int i = 0; i < IM_ARRAYSIZE(comboItems); i++) {
                        bool isSelected = (currentSelection == i);
                        if (ImGui::Selectable(comboItems[i], isSelected)) {
                            currentSelection = i;  // Update the current selection based on user input
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();  // Keep the default selection focused
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                if (ImGui::Button("Launch")) {
                    switch (currentSelection) {
                    case 0:
                        // Launch GTA 5 normally
                        ProcessHandler::LaunchGTA5(true);
                        break;
                    case 1:
                        // Launch GTA 5 without anticheat (used for modded game)
                        ProcessHandler::LaunchGTA5(false);
                        break;
                    case 2:
                        // Launch FiveM
                        ProcessHandler::LaunchFiveM();
                        break;
                    default:
                        ImGui::Text("[Error] Invalid selection");
                        break;
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Launches the selected game.");
                }
                


                ImGui::SameLine();
            }


            if (ImGui::Button("Exit GTA 5")) {
                // Toggle the status of whether GTA 5 or FiveM is running
                gta5NotRunning = !ProcessHandler::TerminateGTA5();
                fivemNotRunning = !ProcessHandler::TerminateGTA5();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Terminates the GTA 5 or FiveM process.");
            }

            if (gta5NotRunning) {
                ImGui::Text("GTA5.exe isn't running.");
            }
            else if (fivemNotRunning) {
                ImGui::Text("FiveM.exe isn't running.");
            }


            ImGui::Checkbox("Demo Window", &showImGuiDebugWindow);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If enabled, the ImGui demo window will be shown.");
            }

            if (showImGuiDebugWindow) {
                ImGui::ShowDemoWindow();
            }




            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Cookie Clicker");
            ImGui::Spacing();
            ImGui::Spacing();


            if (ImGui::Button("Click for points")) {
                clickedTimes++;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to add a point.\nCurrent points: %d", clickedTimes);
            }

            ImGui::Text("Points: %d", clickedTimes);


            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImVec4 playerColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for player
            ImVec4 aiColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);     // Red for AI
            ImVec4 drawColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray for Draw
            TicTacToe::DrawTicTacToeInsideWindow(playerColor, aiColor, drawColor);



            ImGui::End();

            if (!showWindow) {
                running = false;
            }




            ImGui::Render();

            constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
            device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
            if (render_target_view != nullptr) {
                device_context->ClearRenderTargetView(render_target_view, color);
            }

            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            swap_chain->Present(1U, 0U);
        }
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

    if (swap_chain) swap_chain->Release();
    if (device_context) device_context->Release();
    if (device) device->Release();
    if (render_target_view) render_target_view->Release();


    DestroyWindow(window);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}