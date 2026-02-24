#include "errors.h"
#include "logger.h"

void logError(ErrorCode code, const char* sensorName) {
    const char* errorMsg;
    
    switch(code) {
        case ERR_DEVICE_NOT_FOUND:
            errorMsg = "Device not found";
            break;
        case ERR_SENSOR_READ_FAILED:
            errorMsg = "Sensor read failed";
            break;
        case ERR_I2C_COMMUNICATION_FAILED:
            errorMsg = "I2C communication failed";
            break;
        case ERR_TIMEOUT:
            errorMsg = "Operation timeout";
            break;
        case ERR_INVALID_DATA:
            errorMsg = "Invalid data received";
            break;
        default:
            errorMsg = "Unknown error";
    }
    
    logError(errorMsg, sensorName);
}