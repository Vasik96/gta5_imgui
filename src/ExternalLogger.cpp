#include "ExternalLogger.h"
#include <fstream>
#include <windows.h>
#include <direct.h>
#include <vector>

namespace ExternalLogger {
    std::string logFilePath;

    void InitializeExternalLogger() {
        // Get the path to the system's temporary directory
        wchar_t tempPath[MAX_PATH];
        if (GetTempPathW(MAX_PATH, tempPath)) {
            // Convert the wide string to a standard string
            std::wstring widePath(tempPath);
            std::wstring wideLogFilePath = widePath + L"external_logs.txt";

            // Convert wideLogFilePath to narrow string using WideCharToMultiByte
            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideLogFilePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (bufferSize > 0) {
                std::vector<char> buffer(bufferSize);
                WideCharToMultiByte(CP_UTF8, 0, wideLogFilePath.c_str(), -1, buffer.data(), bufferSize, nullptr, nullptr);
                logFilePath = std::string(buffer.data());
            }
            else {
                // Handle error in conversion if needed
                return;
            }

            // Clear the log file at the end of the method
            std::ofstream outFile(logFilePath, std::ios::trunc);
            if (outFile.is_open()) {
                outFile.close();  // Close the file after clearing its contents
            }
        }
    }

    // Method to log messages to the external log file
    void LogExternal(const std::string& message) {
        // If the log file path is not set, initialize the external logger
        if (logFilePath.empty()) {
            InitializeExternalLogger();
        }

        // Open the file in append mode and write the log message
        std::ofstream outFile(logFilePath, std::ios::app);
        if (outFile.is_open()) {
            outFile << message << std::endl;
            outFile.close();
        }
    }
}
