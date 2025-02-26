#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>



namespace System {
	void ChangeResolution(int width, int height);
	std::vector<std::string> GetAvailableResolutions();
	int GetCurrentResolutionIndex(const std::vector<std::string>& availableResolutions);
}