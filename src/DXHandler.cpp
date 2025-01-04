#include "DXHandler.h"
#include "Logging.h"

bool InitializeDirectX(HWND window, ID3D11Device*& device, ID3D11DeviceContext*& device_context, IDXGISwapChain*& swap_chain, ID3D11RenderTargetView*& render_target_view) {  

    Logging::Log("Initializing DirectX...", 1);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.RefreshRate.Numerator = 0U;
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 4U;
    sd.OutputWindow = window;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL levels[2]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL level{};
    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0U,
        levels, 2U, D3D11_SDK_VERSION, &sd,
        &swap_chain, &device, &level, &device_context))) {
        return false;
    }

    ID3D11Texture2D* back_buffer = nullptr;
    if (FAILED(swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer)))) {
        return false;
    }

    if (FAILED(device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view))) {
        back_buffer->Release();
        return false;
    }

    back_buffer->Release();

    Logging::Log("DirectX initialized", 1);

    return true;
}

void CleanupDirectX(ID3D11Device* device, ID3D11DeviceContext* device_context, IDXGISwapChain* swap_chain, ID3D11RenderTargetView* render_target_view) {
    if (render_target_view) render_target_view->Release();
    if (swap_chain) swap_chain->Release();
    if (device_context) device_context->Release();
    if (device) device->Release();
}
