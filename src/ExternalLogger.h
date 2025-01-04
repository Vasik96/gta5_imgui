#pragma once
#ifndef EXTERNAL_LOGGER_H
#define EXTERNAL_LOGGER_H

#include <string>

namespace ExternalLogger {
    void InitializeExternalLogger(); // Method to initialize the external log file
    void LogExternal(const std::string& message); // Method to log messages
}

#endif // EXTERNAL_LOGGER_H
