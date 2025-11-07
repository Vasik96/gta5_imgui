#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <Windows.h>



#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>




void QueryTotalVRAM(ID3D11Device* device);

bool InitializeDirectX(HWND window,
	ID3D11Device*& device,
	ID3D11DeviceContext*& device_context,
	IDXGISwapChain*& swap_chain, 
	ID3D11RenderTargetView*& render_target_view
);
void CleanupDirectX(ID3D11Device* device,
	ID3D11DeviceContext* device_context,
	IDXGISwapChain* swap_chain, 
	ID3D11RenderTargetView* render_target_view
);

void dx11_reset();



namespace safety_checks {
	void check_dx_validity();
}