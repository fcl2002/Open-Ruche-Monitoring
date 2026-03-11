/**
 *
 * LoRa-E5 communication module for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * Manages OTAA join to TTN, periodic uplinks, downlink command parsing,
 * and calibration/offset persistence via NVS (Preferences).
 *
 * Downlink command protocol (2-byte hex payload):
 *   01 XX  — set uplink interval to XX minutes (1–60)
 *   02 XX  — set temperature offset to (int8_t)XX / 10 °C
 *   03 01  — apply tare (capture current gross weight as zero reference)
 *   03 00  — reset tare to zero
 *   04 XX  — set humidity offset to (int8_t)XX %
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include "payload.h"

// Calibration parameters persisted in NVS across power cycles.
struct LoRaCalibration {
    int16_t  tempOffset;     // Additive temperature compensation (°C * 10, same unit as SensorPayload)
    int8_t   humOffset;      // Additive humidity compensation (%)
    int32_t  tareWeight;     // HX711 reading (g*100) captured at tare time, stored as int32 to allow signed net calculation
    uint32_t sendInterval;   // Uplink interval in milliseconds
};

// Initialise UART to the LoRa-E5 module, restore NVS calibration, and
// start OTAA join.  Call once from setup().
void lora_init();

// Process all incoming LoRa-E5 serial bytes and forward Serial monitor
// input to the module.  Must be called every loop() iteration.
void lora_tick();

// Returns true when the network is joined and the send interval has elapsed.
// Use this to gate sensor readings so they only happen when a send is due.
bool lora_should_send();

// Serialise the payload to a hex string and fire AT+CMSGHEX.
// Resets the internal send timer.  Call only when lora_should_send() == true.
void lora_send(const SensorPayload& payload);

// Current calibration values (updated live by downlink commands).
const LoRaCalibration& lora_calibration();

// True once the "Network joined" confirmation is received from the module.
bool lora_is_joined();

#endif /* LORA_H */
