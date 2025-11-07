#pragma once
#include <iostream>
#include <Windows.h>
#include <unordered_map>
#include <unordered_set>
#include <psapi.h>
#include <gdiplus.h>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "imgui/imgui.h"
#include "Logging.h"

namespace KeyboardInput
{
	extern bool insertKeyPressed;
	extern bool wasInsertKeyDown;

	bool IsInGTA();
	void ProcessRawInput(LPARAM lParam);
	bool IsAltF4Pressed();
	void RegisterRawInput(HWND hwnd);
}