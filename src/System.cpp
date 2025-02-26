#include "System.h"


namespace System {
    void ChangeResolution(int width, int height) {
        DEVMODE dm = {};
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = width;
        dm.dmPelsHeight = height;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
            // Handle resolution change failure
            MessageBox(NULL, L"Failed to change resolution", L"Error", MB_ICONERROR);
        }
    }


    std::vector<std::string> GetAvailableResolutions() {
        std::vector<std::string> resolutions;
        DEVMODE dm = {};
        dm.dmSize = sizeof(dm);

        // Enumerate display settings
        for (int modeNum = 0; EnumDisplaySettings(NULL, modeNum, &dm); modeNum++) {
            // Add resolution to the list if it's not already present
            std::string resolution = std::to_string(dm.dmPelsWidth) + "x" + std::to_string(dm.dmPelsHeight);
            if (std::find(resolutions.begin(), resolutions.end(), resolution) == resolutions.end()) {
                resolutions.push_back(resolution);
            }
        }
        return resolutions;
    }

    int GetCurrentResolutionIndex(const std::vector<std::string>& availableResolutions) {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        std::string currentRes = std::to_string(screenWidth) + "x" + std::to_string(screenHeight);

        // Find the current resolution in the list
        auto it = std::find(availableResolutions.begin(), availableResolutions.end(), currentRes);
        if (it != availableResolutions.end()) {
            return static_cast<int>(std::distance(availableResolutions.begin(), it)); // Explicit cast
        }
        return 0; // Default to the first resolution if not found
    }


}