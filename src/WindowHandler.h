#pragma once
#include <Windows.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param);
bool InitializeWindow(HINSTANCE instance, HWND& window, int& screenWidth, int& screenHeight, int cmd_show);
void CleanupWindow(HWND window);
