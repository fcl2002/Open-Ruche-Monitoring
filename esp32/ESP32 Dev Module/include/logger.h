/**
 *
 * Logger library for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Log levels
enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_NONE = 4  // Disable all logging
};

// Set minimum log level (only logs >= this level will be printed)
void setLogLevel(LogLevel level);

// Log functions
void logDebug(const char* message, const char* context = nullptr);
void logInfo(const char* message, const char* context = nullptr);
void logWarn(const char* message, const char* context = nullptr);
void logError(const char* message, const char* context = nullptr);

// Get current uptime as formatted string
void getUptime(char* buffer, size_t bufferSize);

#endif
