#include "Logging.h"
#include "gui.h"
#include <fstream>
#include <windows.h>
#include <thread>
#include "globals.h"
#include <iostream>
#include <sstream>
#include <string>



namespace Logging {
    static std::vector<std::pair<std::string, int>> logBuffer; // In-memory log buffer
    static bool imguiInitialized = false;

    // File to store logs temporarily if necessary
    static const std::string tempLogFile = "temp_logs.txt";



    void Log(const std::string& message, int errorLevel) {

        if (imguiInitialized) {
            gui::LogImGui(message, errorLevel);
        }
        else {
            // Buffer logs in memory
            logBuffer.emplace_back(message, errorLevel);

            // Optionally, write to a temporary file as well
            std::ofstream outFile(tempLogFile, std::ios::app);
            if (outFile.is_open()) {
                outFile << errorLevel << " " << message << std::endl;
                outFile.close();
            }
        }
    }

    void InitializeImGuiLogging() {
        imguiInitialized = true;

        logBuffer.clear();


        // Also, read and flush logs from the temporary file (if it exists)
        std::ifstream inFile(tempLogFile);
        if (inFile.is_open()) {
            std::string line;
            while (std::getline(inFile, line)) {
                size_t spaceIndex = line.find(' ');
                if (spaceIndex != std::string::npos) {
                    int errorLevel = std::stoi(line.substr(0, spaceIndex));
                    std::string message = line.substr(spaceIndex + 1);
                    gui::LogImGui(message, errorLevel);
                }
            }
            inFile.close();
            std::remove(tempLogFile.c_str()); // Remove the file after flushing logs
        }
    }

    void CleanupTemporaryLogs() {
        logBuffer.clear();
        std::remove(tempLogFile.c_str()); // Ensure the temp file is deleted
    }

    void CheckExternalLogFile() {
        // Get the path to the system's temporary directory
        wchar_t tempPath[MAX_PATH];
        if (!GetTempPathW(MAX_PATH, tempPath)) {
            gui::LogImGui("Error: Unable to get the system's temporary directory", 3); // Error severity
            return;
        }

        // Set the log file path to "C:\\Users\\%USERNAME%\\AppData\\Local\\Temp\\external_logs.txt"
        std::wstring wideLogFilePath(tempPath);
        wideLogFilePath += L"external_logs.txt";

        // Convert the wide string to a narrow string using WideCharToMultiByte
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideLogFilePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (bufferSize <= 0) {
            gui::LogImGui("Error: Conversion failed for log file path", 3); // Error severity
            return;
        }
        std::vector<char> buffer(bufferSize);
        WideCharToMultiByte(CP_UTF8, 0, wideLogFilePath.c_str(), -1, buffer.data(), bufferSize, nullptr, nullptr);
        std::string logFilePath(buffer.data());

        // Open the log file
        std::ifstream logFile(logFilePath);

        if (!logFile.is_open()) {
            gui::LogImGui("Error: Unable to open external_logs.txt", 3); // Error severity
            return;
        }

        std::vector<std::string> linesToKeep;
        std::string line;
        while (std::getline(logFile, line)) {
            if (line.find("//LOGGED") != std::string::npos) {
                // Skip lines already marked as logged
                continue;
            }

            // Log the line
            gui::LogImGui(line, 1); // Info severity

            // Append "//LOGGED" to the line and keep it
            line += " //LOGGED";
            linesToKeep.push_back(line);
        }

        logFile.close();

        // Write updated content back to the file, only keeping lines to be logged
        std::ofstream outFile(logFilePath, std::ios::trunc); // Open file in truncate mode
        if (!outFile.is_open()) {
            gui::LogImGui("Error: Unable to write to external_logs.txt", 3); // Error severity
            return;
        }

        for (const auto& updatedLine : linesToKeep) {
            outFile << updatedLine << '\n';
        }

        outFile.close();
    }



}
