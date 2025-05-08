#pragma once
#ifndef EXTERNAL_LOGGER_H
#define EXTERNAL_LOGGER_H

#include <string>

namespace ExternalLogger
{
    void InitializeExternalLogger(const std::string& serverIP, int port);
    void LogExternal(const std::string& message);
}

#endif // EXTERNAL_LOGGER_H
