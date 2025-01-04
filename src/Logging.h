#pragma once
#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <vector>
#include "ExternalLogger.h"

namespace Logging {
    void Log(const std::string& message, int errorLevel);
    void InitializeImGuiLogging();
    void CleanupTemporaryLogs(); // Cleans up temporary storage
    void CheckExternalLogFile();
}

#endif // LOGGING_H
