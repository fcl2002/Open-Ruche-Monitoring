#include "logger.h"

static LogLevel currentLogLevel = LOG_INFO;  // Default log level

void setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

void getUptime(char* buffer, size_t bufferSize) {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
    
    snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, minutes, seconds);
}

static void printLog(const char* level, const char* message, const char* context) {
    char uptime[16];
    getUptime(uptime, sizeof(uptime));
    
    Serial.print("[");
    Serial.print(uptime);
    Serial.print("] [");
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
