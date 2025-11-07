#include "DXHandler.h"
#include "Logging.h"
#include "globals.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>

double g_totalVRAMGB = 0.0;

void QueryTotalVRAM(ID3D11Device* device)
{
    if (!device) return;

    IDXGIDevice* dxgiDevice = nullptr;
    if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice)))
        return;

    IDXGIAdapter* adapter = nullptr;
    if (FAILED(dxgiDevice->GetAdapter(&adapter)))
    {
        dxgiDevice->Release();
        return;
    }

    DXGI_ADAPTER_DESC desc;
    if (SUCCEEDED(adapter->GetDesc(&desc)))
    {
        g_totalVRAMGB = static_cast<double>(desc.DedicatedVideoMemory) / 1024.0 / 1024.0 / 1024.0;
        std::wcout << L"Total VRAM: " << g_totalVRAMGB << L" GB\n";
    }

    adapter->Release();
    dxgiDevice->Release();
}

bool InitializeDirectX(
    HWND window,
    ID3D11Device*& device,
    ID3D11DeviceContext*& device_context,
    IDXGISwapChain*& swap_chain,
    ID3D11RenderTargetView*& render_target_view
)
{  

    Logging::Log("Initializing DirectX...", 1);

    DXGI_SWAP_CHAIN_DESC sd{ };
    sd.BufferDesc.RefreshRate.Numerator = 0U;
    sd.BufferDesc.RefreshRate.Denominator = 144U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = window;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


    constexpr D3D_FEATURE_LEVEL levels[2]
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL level{ };
    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0U,
        levels,
        2U,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device, &level,
        &device_context)))
    {
        return false;
    }


    ID3D11Texture2D* back_buffer = nullptr;
    if (FAILED(swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer))))
    {
        return false;
    }

    if (FAILED(device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view)))
    {
        back_buffer->Release();
        return false;
    }

    back_buffer->Release();

    Logging::Log("DirectX initialized", 1);

    return true;
}

void CleanupDirectX(
    ID3D11Device* device,
    ID3D11DeviceContext* device_context,
    IDXGISwapChain* swap_chain, 
    ID3D11RenderTargetView* render_target_view)
{
    if (render_target_view) render_target_view->Release();
    if (swap_chain) swap_chain->Release();
    if (device_context) device_context->Release();
    if (device) device->Release();
}

void dx11_reset() {
    Logging::Log("[warning] dx invalid, resetting...", 2);

    ImGui_ImplDX11_InvalidateDeviceObjects();

    if (g_render_target_view) { g_render_target_view->Release(); g_render_target_view = nullptr; }
    if (g_swap_chain) { g_swap_chain->Release();         g_swap_chain = nullptr; }
    if (g_device_context) { g_device_context->Release();     g_device_context = nullptr; }
    if (g_device) { g_device->Release();             g_device = nullptr; }

    if (!InitializeDirectX(window, g_device, g_device_context, g_swap_chain, g_render_target_view))
    {
        Logging::Log("[error] InitializeDirectX failed during dx11_reset()", 3);
        return;
    }

    if (!ImGui_ImplDX11_Init(g_device, g_device_context)) {
        Logging::Log("[error] failed to init imgui_dx11 backend", 3);
    }
    if (!ImGui_ImplDX11_CreateDeviceObjects()) {
        Logging::Log("[error] failed to create imgui device objects", 3);
    }

    Logging::Log("dx reset complete", 1);
}

//checks for invalid stuff and resets/recreates it
void safety_checks::check_dx_validity() {
    // g_device
    // g_device_context
    // g_render_target_view
    // g_swap_chain
    
    bool invalid = false;

    // ======= Basic null checks =======
    if (!g_device || !g_device_context || !g_swap_chain || !g_render_target_view)
        invalid = true;

    // ======= Check device status =======
    if (!invalid)
    {
        HRESULT reason = g_device->GetDeviceRemovedReason();
        if (reason != S_OK)
            invalid = true;
    }

    // ======= Check Present() health =======
    // Test Present without actually syncing or displaying
    if (!invalid)
    {
        HRESULT hr = g_swap_chain->Present(0, DXGI_PRESENT_TEST);
        if (FAILED(hr))
            invalid = true;
    }

    // If *any* fail, rebuild device + swapchain + imgui resources
    if (invalid)
    {
        dx11_reset();
    }

}
