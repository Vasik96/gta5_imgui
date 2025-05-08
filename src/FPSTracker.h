#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <psapi.h>
#include <iostream>
#include <chrono>
#include <string>


void StartEtwSession();
void OpenAndProcess();
void StartFpsCalculation();

namespace GTA
{
	int GetCurrentFPS();
}
void StopEtwSession();