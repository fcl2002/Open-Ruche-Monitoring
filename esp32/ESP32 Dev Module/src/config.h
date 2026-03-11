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

// ── LoRa-E5 UART ─────────────────────────────────────────────
#define LORA_RX_PIN              16          // ESP32 GPIO connected to LoRa-E5 TX
#define LORA_TX_PIN              17          // ESP32 GPIO connected to LoRa-E5 RX
#define LORA_SERIAL_NUM          2           // ESP32 HardwareSerial index
#define LORA_DEFAULT_INTERVAL_MS 5000UL     // Default uplink interval (30 s)

// ── TTN OTAA credentials ─────────────────────────────────────
#define LORA_DEV_EUI  "70B3D57ED0075CEE"
#define LORA_APP_EUI  "0000000000000000"
#define LORA_APP_KEY  "3EEA512460C979FF5FE036D9FAEB077D"

#endif /* CONFIG_H */
