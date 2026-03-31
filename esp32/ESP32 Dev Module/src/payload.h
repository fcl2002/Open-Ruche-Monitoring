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
// Total: 17 bytes
#pragma pack(push, 1)
struct SensorPayload {
    int8_t   ext_humidity;       // External DHT22 humidity (%)
    int16_t  ext_temperature;    // External DHT22 temperature (°C * 10)
    int8_t   int_humidity;       // Internal DHT22 humidity (%)
    int16_t  int_temperature;    // Internal DHT22 temperature (°C * 10)
    int16_t  sonde1_temperature; // DS18B20 sonde 1 (°C * 10)
    int16_t  sonde2_temperature; // DS18B20 sonde 2 (°C * 10)
    uint16_t lux;                // SEN0562 luminosity (lux)
    uint16_t weight;             // HX711 load cell (g * 100)
    uint8_t  hive_status;        // Microphone AI: 1=normal, 2=swarming, 3=missing queen, 0=no data
    uint8_t  camera_status;      // Camera AI: raw uint8 from vision classifier, 0=no data
    uint8_t  battery_v;          // Battery percentage (0–100 %), 0 V = 0%, 4.2 V = 100%
};
#pragma pack(pop)

#endif /* PAYLOAD_H */
