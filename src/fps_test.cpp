/*#include "fps_test.h"
#include <unordered_map>
#include <mutex>
#include "globals.h"

int _fps_counter = 0;
int _fps_display = 0;

static const GUID DXGI_PROVIDER =
{ 0xCA11C036,0x0102,0x4A2D,{0xA6,0xAD,0xF0,0x3C,0xFE,0xD5,0xD3,0xC9} };  // :contentReference[oaicite:4]{index=4}

// Microsoft-Windows-D3D12
static const GUID D3D12_PROVIDER =
{ 0x81BDCB0C,0x4F7E,0x4B6F,{0xAE,0xFE,0xE3,0xF8,0x6B,0x30,0x5F,0x69} };  // :contentReference[oaicite:5]{index=5}

static TRACEHANDLE sessionHandle = 0;
static TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;

// Target process name
const wchar_t* TARGET_NAME = L"GTA5_Enhanced.exe";


DWORD g_gta5_pid = 0;

void UpdateGamePid() {
    DWORD pid = 0;
    HWND hwnd = FindWindow(NULL, L"Grand Theft Auto V");
    if (hwnd) {
        GetWindowThreadProcessId(hwnd, &pid);
        g_gta5_pid = pid;
    }
}


std::unordered_map<int, std::vector<long long>> frameTimestamps;
std::mutex frameMutex;


ULONG WINAPI EventRecordCallback(PEVENT_RECORD pEvent) {

    if (pEvent->EventHeader.ProviderId == DXGI_PROVIDER &&
        pEvent->EventHeader.EventDescriptor.Id == 42  // DXGI Present event
        && pEvent->EventHeader.ProcessId == g_gta5_pid)    // Filter by process ID (GTA5)
    {
        std::cout << "its a valid event\n";

        // Get the current timestamp (in milliseconds)
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();


        // Lock the mutex before modifying the global timestamp map
        std::lock_guard<std::mutex> lock(frameMutex);

        // Store the timestamp for the current PID
        frameTimestamps[pEvent->EventHeader.ProcessId].push_back(timestamp);
    }
    return 0;
}

void StartEtwSession() {
    UpdateGamePid();

    constexpr int PROPERTIES_SIZE = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAX_PATH * sizeof(wchar_t);
    auto properties = (EVENT_TRACE_PROPERTIES*)malloc(PROPERTIES_SIZE);
    ZeroMemory(properties, PROPERTIES_SIZE);
    properties->Wnode.BufferSize = PROPERTIES_SIZE;
    properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    properties->Wnode.ClientContext = 1;  // QPC
    properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    ULONG status = StartTrace(&sessionHandle, L"GTA5FPS", properties);
    if (status == ERROR_ALREADY_EXISTS) {
        std::wcerr << L"Session already exists. Stopping it...\n";
        ControlTrace(0, L"GTA5FPS", properties, EVENT_TRACE_CONTROL_STOP);
        status = StartTrace(&sessionHandle, L"GTA5FPS", properties);
    }

    if (status != ERROR_SUCCESS) {
        std::cerr << "StartTrace failed: " << status << "\n";
        free(properties);
        exit(1);
    }

    // Enable providers
    status = EnableTraceEx2(sessionHandle, &DXGI_PROVIDER,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION,
        0, 0, 0, nullptr);
    if (status != ERROR_SUCCESS) {
        std::cerr << "EnableTraceEx2 for DXGI failed: " << status << "\n";
    }

    status = EnableTraceEx2(sessionHandle, &D3D12_PROVIDER,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION,
        0, 0, 0, nullptr);
    if (status != ERROR_SUCCESS) {
        std::cerr << "EnableTraceEx2 for D3D12 failed: " << status << "\n";
    }

    free(properties);
}


void OpenAndProcess(int& fpsCounter) {
    EVENT_TRACE_LOGFILE logFile = {};
    logFile.LoggerName = const_cast<LPWSTR>(L"GTA5FPS");
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.EventRecordCallback = (PEVENT_RECORD_CALLBACK)(EventRecordCallback);
    logFile.Context = &fpsCounter;

    traceHandle = OpenTrace(&logFile);
    if (traceHandle == INVALID_PROCESSTRACE_HANDLE) {
        std::cerr << "OpenTrace failed\n";
        exit(1);
    }

    // Process incoming events on this thread
    ProcessTrace(&traceHandle, 1, nullptr, nullptr);
}

void CalculateFps() {
    using namespace std::chrono;

    while (globals::running) {
        std::this_thread::sleep_for(1s);

        auto now = steady_clock::now().time_since_epoch();
        long long now_ms = duration_cast<milliseconds>(now).count();
        long long one_sec_ago = now_ms - 1000;

        std::lock_guard<std::mutex> lock(frameMutex);

        auto it = frameTimestamps.find(g_gta5_pid);
        if (it != frameTimestamps.end()) {
            auto& timestamps = it->second;

            // Remove timestamps older than 1 second
            timestamps.erase(
                std::remove_if(
                    timestamps.begin(),
                    timestamps.end(),
                    [one_sec_ago](long long ts) {
                        return ts < one_sec_ago;
                    }
                ),
                timestamps.end()
            );

            // Set FPS to number of frames in the last second
            _fps_display = static_cast<int>(timestamps.size());
        }
        else {
            _fps_display = 0;  // No frames, no FPS
        }
    }
}



// Start the FPS calculation thread in your main function
void StartFpsCalculation() {
    std::thread fpsThread(CalculateFps);
    fpsThread.detach();
}*/