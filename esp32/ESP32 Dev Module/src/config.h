/**
 *
 * Central configuration for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#ifndef CONFIG_H
#define CONFIG_H

// ── Deep sleep ────────────────────────────────────────────────
#define DEEP_SLEEP_DURATION_S   5
#define uS_TO_S_FACTOR          1000000

// ── I2C pins ──────────────────────────────────────────────────
#define I2C_SDA_PIN             21
#define I2C_SCL_PIN             22

// ── I2C addresses ─────────────────────────────────────────────
#define MMA8451_ADDR            0x1C
#define SEN0562_ADDR            0x23

// ── HX711 ─────────────────────────────────────────────────────
#define HX711_DOUT_PIN          32
#define HX711_SCK_PIN           33
#define HX711_SCALE             30148
#define HX711_OFFSET            134750

// ── DHT22 ─────────────────────────────────────────────────────
#define EXTERNAL_DTH22_DATA_PIN 27
#define INTERNAL_DTH22_DATA_PIN 26

// ── DS18B20 ───────────────────────────────────────────────────
#define ONE_WIRE_BUS            4

// ── Sensor error sentinels ────────────────────────────────────
#define SENSOR_ERROR_VALUE      -32768
#define SENSOR_LUX_ERROR_VALUE  0
#define ACCEL_ERROR_VALUE       -32768

#endif /* CONFIG_H */
