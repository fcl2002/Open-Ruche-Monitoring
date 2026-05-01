#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "errors.h"
#include "logger.h"
#include "payload.h"
#include "sleep.h"
#include "lora.h"
#include "boards.h"
#if DEBUG_MODE
    RTC_DATA_ATTR int bootCount = 0;
#endif
RTC_DATA_ATTR int dormant_streak = 0;  // consecutive dormant boots; reset on active cycle

static void shutdown_and_sleep(uint32_t duration_s) {
    logInfo("Preparing LoRa module for sleep...", "SYSTEM");
    lora_power_off();
    digitalWrite(VREG_3V3_PIN, LOW);
    logInfo("Voltage regulator 3.3V OFF", "SLEEP");
#if DEBUG_MODE
    Serial.flush();
#endif
    enter_deep_sleep(duration_s);
}

void setup() {
#if DEBUG_MODE
    Serial.begin(115200);
    delay(100);
#endif
#if DEBUG_MODE
    ++bootCount;
#endif
#if DEBUG_MODE
    Serial.println("\n----------------------");
    Serial.println(String(bootCount) + "th Boot");
#endif

    buzzer_boot_beep();
    sleep_gpio_release();

    pinMode(VREG_5V_PIN, OUTPUT);
    digitalWrite(VREG_5V_PIN, LOW);
    logInfo("Voltage regulator 5V OFF", "SLEEP");

    pinMode(VREG_3V3_PIN, OUTPUT);
    digitalWrite(VREG_3V3_PIN, HIGH);
    logInfo("Voltage regulator 3.3v ON", "SLEEP");

    setLogLevel(LOG_INFO);
#if DEBUG_MODE
    print_wakeup_reason();
#endif

    logInfo("Open Ruche Monitoring System Starting...", "SYSTEM");
    // 2. Quick night check (lux only)
    // Bring up I2C just long enough to decide whether we should remain awake.
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    delay(20);
    uint16_t lux = read_sen0562();

    char check_msg[96];
    snprintf(check_msg, sizeof(check_msg),
             "Quick  check — lux=%u (threshold=%u)",
             lux,
             LUX_ACTIVITY_THRESHOLD);
    logInfo(check_msg, "SYSTEM");

    if (lux < LUX_ACTIVITY_THRESHOLD) {
        ++dormant_streak;
        snprintf(check_msg, sizeof(check_msg),
                 "Dormant mode (lux=%u) — sleeping %lus [streak %d]",
                 lux,
                 (unsigned long)DEEP_SLEEP_DORMANT_S,
                 dormant_streak);
        logInfo(check_msg, "SYSTEM");
        lora_power_off();
        digitalWrite(VREG_3V3_PIN, LOW);
        digitalWrite(VREG_5V_PIN, LOW);
        logInfo("Voltage regulator 3.3V OFF", "SLEEP");
        logInfo("Voltage regulator 5V OFF", "SLEEP");
#if DEBUG_MODE
        Serial.flush();
#endif
        enter_deep_sleep(DEEP_SLEEP_DORMANT_S);
        return;
    } else {
        dormant_streak = 0;
    }

    // 3. Full wakeup (daytime)
    // We already spent ~200 ms on quick lux check; wait the remaining time
    // for DHT22/DS18B20 stabilization before reading them.
    delay(1800);

#if DEBUG_MODE
    Serial.println("[DEBUG] Starting sensors_init...");
#endif
    sensors_init();
#if DEBUG_MODE
    Serial.println("[DEBUG] sensors_init done");
#endif
    boards_init();

    DHT22Result intr = read_dht22(internal_dht, "Internal DHT22");
    int16_t s1 = read_ds18b20_sonde(sonde1, "DS18B20 Sonde 1");
    int16_t s2 = read_ds18b20_sonde(sonde2, "DS18B20 Sonde 2");

    logInfo("Daylight confirmed — starting uplink cycle", "SYSTEM");

    lora_init();

    // Wait for OTAA join — the LoRa-E5 responds quickly if session is still active.
    logInfo("Waiting for LoRa network join...", "SYSTEM");
    const unsigned long JOIN_TIMEOUT_MS = 15000;
    unsigned long deadline = millis() + JOIN_TIMEOUT_MS;
    while (!lora_is_joined() && millis() < deadline) {
        lora_tick();
        delay(50);
    }

    if (!lora_is_joined()) {
        logError(ERR_TIMEOUT, "LoRa join");
        shutdown_and_sleep(DEEP_SLEEP_LORA_S);
        return;
    }

    logInfo("Reading sensors...", "SYSTEM");

    const LoRaCalibration& cal = lora_calibration();
    SensorPayload payload = {0};

    // External DHT22
    DHT22Result ext = read_dht22(external_dht, "External DHT22");
    payload.ext_humidity = (int8_t)constrain(ext.humidity + cal.humOffset, 0, 100);
    payload.ext_temperature = (ext.temperature != SENSOR_ERROR_VALUE)
                                  ? ext.temperature + cal.tempOffset
                                  : (int16_t)SENSOR_ERROR_VALUE;

    // Internal DHT22 — apply humidity and temperature offsets
    payload.int_humidity = (int8_t)constrain(intr.humidity + cal.humOffset, 0, 100);
    payload.int_temperature = (intr.temperature != SENSOR_ERROR_VALUE)
                                  ? intr.temperature + cal.tempOffset
                                  : (int16_t)SENSOR_ERROR_VALUE;

    // DS18B20 sondes — apply temperature offset
    payload.sonde1_temperature = (s1 != SENSOR_ERROR_VALUE)
                                     ? s1 + cal.tempOffset
                                     : (int16_t)SENSOR_ERROR_VALUE;

    payload.sonde2_temperature = (s2 != SENSOR_ERROR_VALUE)
                                     ? s2 + cal.tempOffset
                                     : (int16_t)SENSOR_ERROR_VALUE;

    // Luminosity
    payload.lux = lux;

    // Weight: net weight after tare (g*100)
    payload.weight = read_hx711();

    digitalWrite(VREG_3V3_PIN, LOW);
    logInfo("Voltage regulator 3.3V OFF", "SLEEP");

    pinMode(VREG_5V_PIN, OUTPUT);
    digitalWrite(VREG_5V_PIN, HIGH);
    logInfo("Voltage regulator 5V ON", "SLEEP");

    deadline = millis() + AI_READ_TIME;
    while (millis() < deadline) {
        payload.audio = read_audio();
        payload.camera = read_camera();
    }

    digitalWrite(VREG_5V_PIN, LOW);
    logInfo("Voltage regulator 5V OFF", "SLEEP");

    // Battery voltage via ADC (GPIO35)
    payload.battery_v = read_battery_v();

    lora_send(payload);

    // Listen long enough to cover RX1/RX2 windows even with scheduling jitter.
    const unsigned long DOWNLINK_LISTEN_MS = 10000;
    logInfo("Listening for downlink window...", "SYSTEM");
    deadline = millis() + DOWNLINK_LISTEN_MS;
    while (millis() < deadline) {
        lora_tick();
        delay(50);
    }
    logInfo("Downlink window closed", "SYSTEM");
    shutdown_and_sleep(lora_sleep_duration_s());
}

void loop() {

}