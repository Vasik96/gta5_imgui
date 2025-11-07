#pragma once
#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include <string>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>



namespace ProcessHandler {
    bool TerminateGTA5();
    std::string GetCurrentSession();
    bool IsProcessRunning(const wchar_t* processName);
    void ResetSession();
    void LaunchFiveM();
    void LaunchGTA5(bool AntiCheatEnabled);
    void DesyncFromGTAOnline();
    DWORD FindProcessIdByWindow(const std::wstring& windowTitle);
    DWORD GetPIDByProcessName(const wchar_t* processName);
    std::string GetProcessNameFromPID(DWORD pid);
}


#endif