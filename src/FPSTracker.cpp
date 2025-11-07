#include "FPSTracker.h"
#include "globals.h"

#include <evntrace.h>
#include <evntcons.h>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <deque>
#include "ProcessHandler.h"

using namespace std::chrono;

static const GUID DXGI_PROVIDER = { 0xCA11C036,0x0102,0x4A2D,{0xA6,0xAD,0xF0,0x3C,0xFE,0xD5,0xD3,0xC9} };
static const GUID D3D12_PROVIDER = { 0x81BDCB0C,0x4F7E,0x4B6F,{0xAE,0xFE,0xE3,0xF8,0x6B,0x30,0x5F,0x69} };

static TRACEHANDLE sessionHandle = 0;
static TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;

static std::unordered_map<DWORD, std::vector<long long>> frameTimestamps;
static std::mutex frameMutex;

DWORD g_tracked_pid = 0;
static int fps_display = 0;

static std::wstring gta_window_title = L"Grand Theft Auto V";
static std::wstring fivem_window_title = L"FiveM";

bool found_gta5 = false;

void TrackGameProcessLoop() {
    while (globals::running) {
        if (!found_gta5) {
            DWORD gta_pid = ProcessHandler::FindProcessIdByWindow(gta_window_title);
            DWORD fivem_pid = ProcessHandler::FindProcessIdByWindow(fivem_window_title);

            DWORD found_pid = gta_pid ? gta_pid : fivem_pid;


            if (found_pid && found_pid != g_tracked_pid) {
                {
                    std::lock_guard<std::mutex> lock(frameMutex);
                    g_tracked_pid = found_pid;
                    frameTimestamps[g_tracked_pid].clear(); // reset timestamps for new PID
                }
                std::wcout << L"Tracked PID updated to: " << g_tracked_pid << L"\n";

                found_gta5 = true;
            }
        }

        std::wcout << L"Tracking current PID: " << g_tracked_pid << std::endl;
        std::this_thread::sleep_for(5s);
    }
}


static double last_frametime_ms = 0.0;
static std::deque<double> frametime_history;
constexpr int SMOOTH_FRAMES = 4; // rolling average window

ULONG WINAPI EventRecordCallback(PEVENT_RECORD pEvent) {
    if ((pEvent->EventHeader.ProviderId == DXGI_PROVIDER &&
        pEvent->EventHeader.EventDescriptor.Id == 42))
    {
        DWORD pid = pEvent->EventHeader.ProcessId;
        if (pid != g_tracked_pid)
            return 0;

        // ETW timestamp in 100ns units -> ms
        long long ts_ms = pEvent->EventHeader.TimeStamp.QuadPart / 10000LL;

        static thread_local long long last_ts_ms = 0;
        if (last_ts_ms > 0) {
            double delta = static_cast<double>(ts_ms - last_ts_ms);

            if (delta >= 1.0) { // filter out ETW jitter
                frametime_history.push_back(delta);
                if (frametime_history.size() > SMOOTH_FRAMES)
                    frametime_history.pop_front();

                // rolling average
                double avg = 0.0;
                for (double d : frametime_history) avg += d;
                avg /= frametime_history.size();

                last_frametime_ms = avg;

                
                std::cout << "[FPS DEBUG] PID " << pid
                    << " frametime=" << last_frametime_ms
                    << " ms, FPS=" << (1000.0 / last_frametime_ms) << "\n";
            }
        }

        last_ts_ms = ts_ms;
    }

    return 0;
}



void StartEtwSession() {

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


void StartPerformanceCalculations() {
    std::thread pidTrackerThread(TrackGameProcessLoop);
    pidTrackerThread.detach();
}

double GTA::GetCurrentFrametimeMs() {
    return last_frametime_ms;
}

int GTA::GetCurrentFPS() {
    if (last_frametime_ms > 0.0)
        return static_cast<int>(1000.0 / last_frametime_ms);
    return 0;
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