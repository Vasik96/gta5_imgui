#pragma once
#include <iostream>
#include <Windows.h>
#include <unordered_map>
#include <unordered_set>

#include "imgui/imgui.h"
#include "Logging.h"

namespace KeyboardInput
{
	extern bool insertKeyPressed;
	extern bool wasInsertKeyDown;

	bool IsInGTA();
	void ProcessRawInput(LPARAM lParam);
	void RegisterRawInput(HWND hwnd);
}