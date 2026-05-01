#ifndef ERRORS_H
#define ERRORS_H

#include <Arduino.h>
#include "logger.h"

enum ErrorCode {
    ERR_NONE = 0,
    ERR_DEVICE_NOT_FOUND,
    ERR_SENSOR_READ_FAILED,
    ERR_I2C_COMMUNICATION_FAILED,
    ERR_TIMEOUT,
    ERR_INVALID_DATA,
    ERR_LOW_MEMORY
};

void logError(ErrorCode code, const char* sensorName = nullptr);

#endif