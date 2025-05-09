#ifndef GUI_H
#define GUI_H

#include <d3d11.h>
#include <string>
#include <Windows.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>


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
}



#endif // GUI_H
