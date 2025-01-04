#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <Windows.h>

bool InitializeDirectX(HWND window, ID3D11Device*& device, ID3D11DeviceContext*& device_context, IDXGISwapChain*& swap_chain, ID3D11RenderTargetView*& render_target_view);
void CleanupDirectX(ID3D11Device* device, ID3D11DeviceContext* device_context, IDXGISwapChain* swap_chain, ID3D11RenderTargetView* render_target_view);
