#pragma once
#ifndef FORMATTEDINFO_H
#define FORMATTEDINFO_H

#include <string>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

namespace FormattedInfo {
    std::string GetFormattedTime();
    std::string GetFormattedDate();
    std::string GetFormattedCPUUsage();
    std::string GetFormattedRAMUsage();
    std::string GetFormattedRP();
}

#endif
