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
#define DEEP_SLEEP_DURATION_S   15       // Normal cycle: sleep and wake interval (seconds)
#define DEEP_SLEEP_LORA_S       60      // Normal cycle: sleep and wake interval (seconds)
#define DEEP_SLEEP_DORMANT_S    3600     // Dormant mode: 60 min between checks
#define DORMANT_STREAK_MAX      12        // Force active cycle after this many consecutive dormant boots (~12 h)
#define uS_TO_S_FACTOR          1000000
#define AI_READ_TIME            10000    // AI Boards read time

// ── Activity thresholds ───────────────────────────────────────
// Both conditions must be true to enter the active (uplink) cycle.
// Below either threshold the system stays in dormant deep sleep.
#define LUX_ACTIVITY_THRESHOLD   100   // lux  — minimum daylight level
#define TEMP_ACTIVITY_THRESHOLD  150   // °C×10 — 15.0 °C minimum

// ── Battery ADC ───────────────────────────────────────────────
#define BATTERY_ADC_PIN         35          // ADC1_CH7 — battery voltage sense (V_BAT = 1.435 * raw/4095 * 3.3)

// ── Buzzer ────────────────────────────────────────────────────
#define BUZZER_PIN              19          // Active buzzer enable (HIGH = on)

// ── Voltage regulators ───────────────────────────────────────
#define VREG_3V3_PIN              12          // Regulator 1 enable (HIGH = on)
#define VREG_5V_PIN               13          // Regulator 2 enable (HIGH = on)

// ── I2C pins ──────────────────────────────────────────────────
#define I2C_SDA_PIN             21
#define I2C_SCL_PIN             22

// ── I2C addresses ─────────────────────────────────────────────
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

// ── Microphone AI board UART (UART1 hardware) ────────────────
#define AI_RX_PIN           14          // ESP32 GPIO receiving microphone AI TX
#define AI_UART_NUM         1           // HardwareSerial index (UART1)
#define AI_BAUD             9600
#define AI_READ_TIMEOUT_MS  500

// ── Camera AI board UART (SoftwareSerial — HW UARTs exhausted) ─
// UART0=USB, UART1=microphone, UART2=LoRa → no HW UART left.
#define CAM_RX_PIN          25          // ESP32 GPIO receiving camera AI TX
#define CAM_BAUD            115200
#define CAM_READ_TIMEOUT_MS 500

// ── LoRa-E5 UART ─────────────────────────────────────────────
#define LORA_RX_PIN              16          // ESP32 GPIO connected to LoRa-E5 TX
#define LORA_TX_PIN              17          // ESP32 GPIO connected to LoRa-E5 RX
#define LORA_SERIAL_NUM          2           // ESP32 HardwareSerial index
#define LORA_DEFAULT_INTERVAL_MS (DEEP_SLEEP_DURATION_S * 1000UL)  // Synchronized with ESP wake cycle

// ── TTN OTAA credentials ─────────────────────────────────────
#define LORA_DEV_EUI  "70B3D57ED0075CEE"
#define LORA_APP_EUI  "0000000000000000"
#define LORA_APP_KEY  "3EEA512460C979FF5FE036D9FAEB077D"

#endif /* CONFIG_H */
