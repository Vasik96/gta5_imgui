#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <psapi.h>
#include <iostream>
#include <chrono>
#include <string>


void StartEtwSession();
void OpenAndProcess();
void StartPerformanceCalculations();

namespace GTA
{
    double GetCurrentFrametimeMs();
    int GetCurrentFPS();
}
void StopEtwSession();

extern DWORD g_tracked_pid;