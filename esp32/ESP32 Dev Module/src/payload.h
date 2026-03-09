/**
 *
 * Sensor payload definition for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <stdint.h>

// Packed struct — byte layout matches the radio transmission format.
// Total: 19 bytes
#pragma pack(push, 1)
struct SensorPayload {
    int8_t   ext_humidity;       // External DHT22 humidity (%)
    int16_t  ext_temperature;    // External DHT22 temperature (°C * 10)
    int8_t   int_humidity;       // Internal DHT22 humidity (%)
    int16_t  int_temperature;    // Internal DHT22 temperature (°C * 10)
    int16_t  sonde1_temperature; // DS18B20 sonde 1 (°C * 10)
    int16_t  sonde2_temperature; // DS18B20 sonde 2 (°C * 10)
    uint16_t lux;                // SEN0562 luminosity (lux)
    int16_t  accel_x;            // MMA8451 X-axis (m/s² * 10)
    int16_t  accel_y;            // MMA8451 Y-axis (m/s² * 10)
    int16_t  accel_z;            // MMA8451 Z-axis (m/s² * 10)
    uint16_t weight;             // HX711 load cell (g * 100)
};
#pragma pack(pop)

#endif /* PAYLOAD_H */
