#ifndef GUI_H
#define GUI_H

#include <d3d11.h>
#include <string>
#include <Windows.h>
#include <mutex>
#include <chrono>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>

#include "globals.h"

void InitializeImGui(
    HWND window, 
    ID3D11Device* device, 
    ID3D11DeviceContext* device_context
);
void RenderImGui(
    bool p_gta5NotRunning,
    bool p_fivemNotRunning,
    HWND window
);


void guiLoadImage(
    ID3D11Device* device,
    const char* imagePath, 
    ID3D11ShaderResourceView** textureView
);

namespace gui
{
    void LogImGui(const std::string& message, int errorLevel);

    enum class GuiTabs
    {
        TAB_MAIN,
        TAB_UTILITY,
        TAB_NETWORK,
        TAB_MODS,
        TAB_SYSTEM,
        TAB_INFO,
        TAB_EXTRAS,
        TAB_LOGS
    };
}

extern bool has_resolution_changed_for_ssaa;



class AlertNotification {
private:
    std::mutex mtx;

    std::optional<std::string> message;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::milliseconds notification_time { 2500 };

public:
    void CreateNotification(const std::string& text) {
        std::lock_guard<std::mutex> lock(mtx);
        message.reset();

        message = text;
        start_time = std::chrono::steady_clock::now();
    }

    void RemoveNotifications() {
        std::lock_guard<std::mutex> lock(mtx);
        message.reset();
    }

    void Draw() {
        std::lock_guard<std::mutex> lock(mtx);

        if (message) {
            auto now = std::chrono::steady_clock::now();
            if (now - start_time > notification_time) {
                message.reset();
                return;
            }

            ImGuiWindowFlags window_flags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoMouseInputs |
                ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoMove;

            ImVec2 text_size = ImGui::CalcTextSize(message->c_str());
            ImGui::SetNextWindowPos(ImVec2((screenWidth / 2) - text_size.x / 2, screenHeight * 0.05f));
            ImGui::SetNextWindowSize(ImVec2(text_size.x + 20, text_size.y + 20));
            ImGui::Begin("##NotificationWindow", nullptr, window_flags);
            ImGui::Text(message->c_str());
            ImGui::End();
        }
    }
};

extern AlertNotification g_alert_notification;


#endif // GUI_H
