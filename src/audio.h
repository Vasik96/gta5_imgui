#pragma once
#include <iostream>
#include <Windows.h>


#pragma comment(lib, "winmm.lib")


inline std::wstring GetExecutablePath()
{
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	std::wstring exePath(path);
	size_t pos = exePath.find_last_of(L"\\/");
	return exePath.substr(0, pos);
}

// Function to play a WAV file
inline void PlayWavFile(const std::wstring& fileName)
{
	std::wstring fullPath = GetExecutablePath() + L"\\" + fileName;
	PlaySoundW(fullPath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}
