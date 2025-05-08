#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <dxgi.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <string>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>
#include <numeric>
#include <fstream>
#include <sstream>
#include <string>


#include "FormattedInfo.h"
#include "Logging.h"
#include "globals.h"


bool globals::isTimerRunning = false;



namespace FormattedInfo
{
    std::string lastCpuUsage = "0%";
    std::string lastRamUsage = "0/0GB";
    std::string lastProcessCount = "0";

    std::chrono::steady_clock::time_point lastCpuUpdate = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastRamUpdate = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastProcessUpdate = std::chrono::steady_clock::now();






    // Gets the system time in a formatted way (example: 15:30)
    std::string GetFormattedTime()
    {
        // Get current system time
        auto now = std::chrono::system_clock::now();
        std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

        // Convert to local time safely using localtime_s
        std::tm localTime;
        localtime_s(&localTime, &nowTime);  // thread-safe version of localtime

        // Format time as HH:MM
        std::ostringstream formattedTime;
        formattedTime << std::setw(2) << std::setfill('0') << localTime.tm_hour << ":"
            << std::setw(2) << std::setfill('0') << localTime.tm_min;

        return formattedTime.str();
    }

    std::string GetFormattedDate()
    {
        char buffer[11]; // Buffer to store the formatted date (e.g., "25/12/2024\0")
        std::time_t now = std::time(nullptr); // Get the current time
        std::tm localTime; // To store the local time
        localtime_s(&localTime, &now); // Convert to local time (global version of localtime_s)
        std::strftime(buffer, sizeof(buffer), "%d/%m/%Y", &localTime); // Format as DD/MM/YYYY
        return std::string(buffer);
    }




    //1.5s - reasonable cooldown to prevent flashing info in imgui and at the same time retrieve accurate information
    std::string GetFormattedCPUUsage()
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - lastCpuUpdate;

        if (elapsed.count() >= 1.5f)
        {
            FILETIME idleTime, kernelTime, userTime;
            if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
            {
                ULONGLONG idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
                ULONGLONG kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
                ULONGLONG user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

                static ULONGLONG prevIdle = 0, prevKernel = 0, prevUser = 0;

                ULONGLONG deltaIdle = idle - prevIdle;
                ULONGLONG deltaKernel = kernel - prevKernel;
                ULONGLONG deltaUser = user - prevUser;

                prevIdle = idle;
                prevKernel = kernel;
                prevUser = user;

                ULONGLONG total = deltaKernel + deltaUser;
                if (total == 0)
                {
                    lastCpuUsage = "0%";
                }
                else 
                {
                    int cpuUsage = (int)((deltaKernel + deltaUser - deltaIdle) * 100 / total);
                    lastCpuUsage = std::to_string(cpuUsage) + "%";
                }
            }

            lastCpuUpdate = now;
        }

        return lastCpuUsage;
    }


    std::string GetFormattedRAMUsage()
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - lastRamUpdate;

        if (elapsed.count() >= 1.5f)
        {
            MEMORYSTATUSEX memStatus;
            memStatus.dwLength = sizeof(MEMORYSTATUSEX);
            if (GlobalMemoryStatusEx(&memStatus))
            {
                double totalMemory = static_cast<double>(memStatus.ullTotalPhys) / (1024 * 1024 * 1024);
                double usedMemory = static_cast<double>(memStatus.ullTotalPhys - memStatus.ullAvailPhys) / (1024 * 1024 * 1024);

                std::ostringstream ramUsage;
                ramUsage << std::fixed << std::setprecision(1) << usedMemory << "/"
                    << std::fixed << std::setprecision(1) << totalMemory << "GB";
                lastRamUsage = ramUsage.str();
            }

            lastRamUpdate = now;
        }

        return lastRamUsage;
    }

    std::string GetFormattedRP()
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - lastProcessUpdate;

        if (elapsed.count() >= 1.5f)
        {
            DWORD processIds[1024];
            DWORD bytesReturned;
            if (EnumProcesses(processIds, sizeof(processIds), &bytesReturned))
            {
                DWORD processCount = bytesReturned / sizeof(DWORD);
                lastProcessCount = std::to_string(processCount);
            }

            lastProcessUpdate = now;
        }

        return lastProcessCount;
    }




    std::string GetLogFromFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return "Error: Unable to open file at path: " + filePath;
        }

        std::stringstream buffer;
        buffer << file.rdbuf(); // Read the entire file into the buffer
        return buffer.str();    // Return the contents of the file as a string
    }



    std::string GetFormattedTimer()
    {
        if (!globals::isTimerRunning)
        {
            return "0m 0s";
        }
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - timerStart);
        int minutes = static_cast<int>(duration.count() / 60);
        int seconds = static_cast<int>(duration.count() % 60);
        return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
    }


    std::string GetFormattedMenuUptime()
    {
        using namespace std::chrono;
        auto now = steady_clock::now();
        auto elapsed = duration_cast<seconds>(now - globals::menu_uptime).count();

        int hrs = int(elapsed / 3600);
        int mins = int((elapsed % 3600) / 60);
        int secs = int(elapsed % 60);

        char buf[32];
        snprintf(buf, sizeof(buf), "%dh %dm %ds", hrs, mins, secs);
        return std::string(buf);
    }




}
