#include "FPSTracker.h"
#include "globals.h"

#include <evntrace.h>
#include <evntcons.h>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <mutex>

static const GUID DXGI_PROVIDER = { 0xCA11C036,0x0102,0x4A2D,{0xA6,0xAD,0xF0,0x3C,0xFE,0xD5,0xD3,0xC9} };
static const GUID D3D12_PROVIDER = { 0x81BDCB0C,0x4F7E,0x4B6F,{0xAE,0xFE,0xE3,0xF8,0x6B,0x30,0x5F,0x69} };

static TRACEHANDLE sessionHandle = 0;
static TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;

static std::unordered_map<DWORD, std::vector<long long>> frameTimestamps;
static std::mutex frameMutex;

static DWORD tracked_pid = 0;
static int fps_display = 0;

static std::wstring gta_window_title = L"Grand Theft Auto V";
static std::wstring fivem_window_title = L"FiveM";

void SetTargetWindows(const std::wstring& gtaWindow, const std::wstring& fivemWindow) {
    gta_window_title = gtaWindow;
    fivem_window_title = fivemWindow;
}

DWORD FindProcessIdByWindow(const std::wstring& windowTitle) {
    HWND hwnd = FindWindow(NULL, windowTitle.c_str());
    if (!hwnd) return 0;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    std::cout << "pid: " + std::to_string(pid) + "\n"; // this is correct
    return pid;
}

void UpdateTrackedPid() {
    DWORD gta_pid = FindProcessIdByWindow(gta_window_title);
    DWORD fivem_pid = FindProcessIdByWindow(fivem_window_title);

    if (gta_pid) {
        tracked_pid = gta_pid;
    }
    else if (fivem_pid) {
        tracked_pid = fivem_pid;
    }
    else {
        tracked_pid = 0;
    }
}

ULONG WINAPI EventRecordCallback(PEVENT_RECORD pEvent) {
    std::cout << "eventrecordcallback called\n"; //why is it not called?
    std::cout << "Event PID: " << pEvent->EventHeader.ProcessId << "\n";

    if (pEvent->EventHeader.ProviderId == DXGI_PROVIDER &&
        pEvent->EventHeader.EventDescriptor.Id == 42 &&
        pEvent->EventHeader.ProcessId == tracked_pid)
    {

        std::cout << "valid event\n";

        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        std::lock_guard<std::mutex> lock(frameMutex);
        frameTimestamps[pEvent->EventHeader.ProcessId].push_back(timestamp);
    }
    return 0;
}

void StartEtwSession() {
    UpdateTrackedPid();

    std::cout << "Tracked PID: " << tracked_pid << "\n";
    if (tracked_pid == 0) {
        std::cerr << "No valid game process found.\n";
        return;
    }

    constexpr int PROPERTIES_SIZE = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAX_PATH * sizeof(wchar_t);
    auto properties = (EVENT_TRACE_PROPERTIES*)malloc(PROPERTIES_SIZE);
    ZeroMemory(properties, PROPERTIES_SIZE);

    properties->Wnode.BufferSize = PROPERTIES_SIZE;
    properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    properties->Wnode.ClientContext = 1;
    properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    ULONG status = StartTrace(&sessionHandle, L"GTA5FPS", properties);
    if (status == ERROR_ALREADY_EXISTS) {
        ControlTrace(0, L"GTA5FPS", properties, EVENT_TRACE_CONTROL_STOP);
        status = StartTrace(&sessionHandle, L"GTA5FPS", properties);
    }

    if (status != ERROR_SUCCESS) {
        std::cerr << "StartTrace failed: " << status << "\n";
        free(properties);
        return;
    }

    EnableTraceEx2(sessionHandle, &DXGI_PROVIDER, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION, 0, 0, 0, nullptr);

    EnableTraceEx2(sessionHandle, &D3D12_PROVIDER, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION, 0, 0, 0, nullptr);

    free(properties);

    std::cout << "etw started successfully\n";
}

void OpenAndProcess() {
    std::cout << "OpenAndProcess called\n";


    EVENT_TRACE_LOGFILE logFile = {};
    logFile.LoggerName = const_cast<LPWSTR>(L"GTA5FPS");
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.EventRecordCallback = (PEVENT_RECORD_CALLBACK)EventRecordCallback;

    traceHandle = OpenTrace(&logFile);
    if (traceHandle == INVALID_PROCESSTRACE_HANDLE) {
        std::cerr << "OpenTrace failed\n";
        return;
    }

    ProcessTrace(&traceHandle, 1, nullptr, nullptr);
}

void CalculateFps() {
    using namespace std::chrono;

    while (globals::running) {
        std::this_thread::sleep_for(1s);

        // Step 1: Update the tracked process ID based on which game is running
        UpdateTrackedPid();

        if (tracked_pid == 0) {
            fps_display = 0;  // No game is found, display 0 FPS
            std::this_thread::sleep_for(1s);
            continue;
        }

        // Step 2: Check if FPS is zero (which indicates the ETW session might not be capturing events yet)
        if (fps_display == 0) {
            std::cout << "FPS is 0, restarting ETW session...\n";
            // Step 3: Restart ETW session and give it time to initialize
            StopEtwSession();
            std::this_thread::sleep_for(3s);  // Wait 3 seconds to let the session restart and capture events
            StartEtwSession();

            //crashes the game
            if (globals::fps_process_thread.joinable()) {
                globals::fps_process_thread.join();
            }
            globals::fps_process_thread = std::thread(OpenAndProcess);

            std::this_thread::sleep_for(5s);
        }

        // Step 4: Calculate FPS based on the collected frame timestamps
        long long now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        long long cutoff = now - 1000;

        std::lock_guard<std::mutex> lock(frameMutex);
        auto& timestamps = frameTimestamps[tracked_pid];

        // Remove frames older than 1 second
        timestamps.erase(std::remove_if(timestamps.begin(), timestamps.end(),
            [cutoff](long long t) { return t < cutoff; }), timestamps.end());

        // Calculate FPS as the number of frames in the last 1 second
        fps_display = static_cast<int>(timestamps.size());
    }
}


void StartFpsCalculation() {
    std::cout << "StartFpsCalculation called\n";

    std::thread fpsThread(CalculateFps);
    fpsThread.detach();
}

int GTA::GetCurrentFPS() {
    return fps_display;
}


void StopEtwSession() { // called when app shutdown
    if (traceHandle != INVALID_PROCESSTRACE_HANDLE) {
        // ProcessTrace will exit if CloseTrace is called on its handle
        CloseTrace(traceHandle);
        traceHandle = INVALID_PROCESSTRACE_HANDLE;
        std::cout << "Closed trace handle.\n";
    }

    if (sessionHandle != 0) {
        constexpr int PROPERTIES_SIZE = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAX_PATH * sizeof(wchar_t);
        auto properties = (EVENT_TRACE_PROPERTIES*)malloc(PROPERTIES_SIZE);
        if (properties) {
            ZeroMemory(properties, PROPERTIES_SIZE);
            properties->Wnode.BufferSize = PROPERTIES_SIZE;
            properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            ULONG status = ControlTrace(sessionHandle, nullptr, properties, EVENT_TRACE_CONTROL_STOP);
            if (status != ERROR_SUCCESS) {
                std::cerr << "ControlTrace (stop) failed: " << status << "\n";
            }
            else {
                std::cout << "Stopped ETW session.\n";
            }
            free(properties);
        }
        else {
            std::cerr << "Failed to allocate memory for stopping session properties.\n";
        }
        sessionHandle = 0;
    }
}