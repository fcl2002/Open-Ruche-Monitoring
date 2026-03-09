#include "logger.h"

static LogLevel currentLogLevel = LOG_INFO;  // Default log level

void setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

static void printLog(const char* level, const char* message, const char* context) {
    Serial.print("[");
    Serial.print(level);
    Serial.print("] ");
    
    if (context) {
        Serial.print(context);
        Serial.print(": ");
    }
    
    Serial.println(message);
}

void logDebug(const char* message, const char* context) {
    if (currentLogLevel <= LOG_DEBUG) {
        printLog("DEBUG", message, context);
    }
}

void logInfo(const char* message, const char* context) {
    if (currentLogLevel <= LOG_INFO) {
        printLog("INFO", message, context);
    }
}

void logWarn(const char* message, const char* context) {
    if (currentLogLevel <= LOG_WARN) {
        printLog("WARN", message, context);
    }
}

void logError(const char* message, const char* context) {
    if (currentLogLevel <= LOG_ERROR) {
        printLog("ERROR", message, context);
    }
}
