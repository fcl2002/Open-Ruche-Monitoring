#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "errors.h"
#include "logger.h"
#include "payload.h"
#include "sleep.h"
#include "lora.h"
#include "boards.h"

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int dormant_streak = 0;  // consecutive dormant boots; reset on active cycle

// ── Activity gate ─────────────────────────────────────────────
// Returns true when environmental conditions justify a full uplink
// cycle. Both conditions must be met simultaneously:
//   • lux  >= LUX_ACTIVITY_THRESHOLD  (daylight, bees are active)
//   • temp >= TEMP_ACTIVITY_THRESHOLD (internal average temperature)
// On the first dormant boot (streak == 1) a final payload is still sent
// so the server records the node entering dormant mode. Subsequent dormant
// boots skip the uplink until the gate passes again or the streak limit
// (DORMANT_STREAK_MAX) forces a recovery cycle.
static bool is_active_period(uint16_t lux, int16_t temp_x10) {
    return (lux >= LUX_ACTIVITY_THRESHOLD) && (temp_x10 >= TEMP_ACTIVITY_THRESHOLD);
}

static int16_t median3_i16(int16_t a, int16_t b, int16_t c) {
    if (a > b) {
        int16_t t = a;
        a = b;
        b = t;
    }
    if (b > c) {
        int16_t t = b;
        b = c;
        c = t;
    }
    if (a > b) {
        int16_t t = a;
        a = b;
        b = t;
    }
    return b;
}

static uint16_t median3_u16(uint16_t a, uint16_t b, uint16_t c) {
    if (a > b) {
        uint16_t t = a;
        a = b;
        b = t;
    }
    if (b > c) {
        uint16_t t = b;
        b = c;
        c = t;
    }
    if (a > b) {
        uint16_t t = a;
        a = b;
        b = t;
    }
    return b;
}

static int16_t average_internal_temp_x10(int16_t dht_temp_x10, int16_t sonde1_temp_x10, int16_t sonde2_temp_x10) {
    int32_t sum = 0;
    uint8_t count = 0;

    if (dht_temp_x10 != SENSOR_ERROR_VALUE) {
        sum += dht_temp_x10;
        count++;
    }
    if (sonde1_temp_x10 != SENSOR_ERROR_VALUE) {
        sum += sonde1_temp_x10;
        count++;
    }
    if (sonde2_temp_x10 != SENSOR_ERROR_VALUE) {
        sum += sonde2_temp_x10;
        count++;
    }

    if (count == 0) {
        return SENSOR_ERROR_VALUE;
    }
    return (int16_t)(sum / (int32_t)count);
}

static void shutdown_and_sleep(uint32_t duration_s) {
    logInfo("Preparing LoRa module for sleep...", "SYSTEM");
    lora_sleep();
    digitalWrite(VREG_3V3_PIN, LOW);
    logInfo("Voltage regulator 3.3V OFF", "SLEEP");
    Serial.flush();
    enter_deep_sleep(duration_s);
}

void setup() {
    Serial.begin(115200);
    delay(100);

    ++bootCount;
    Serial.println("\n----------------------");
    Serial.println(String(bootCount) + "th Boot");

    buzzer_boot_beep();
    sleep_gpio_release();

    pinMode(VREG_3V3_PIN, OUTPUT);
    digitalWrite(VREG_3V3_PIN, HIGH);
    logInfo("Voltage regulator 3.3v ON", "SLEEP");

    delay(2000);

    setLogLevel(LOG_INFO);
    print_wakeup_reason();

    logInfo("Open Ruche Monitoring System Starting...", "SYSTEM");

    Serial.println("[DEBUG] Starting sensors_init...");
    sensors_init();
    Serial.println("[DEBUG] sensors_init done");
    boards_init();

    // Quick environmental check
    // Read luminosity and internal temperatures used by the activity gate.
    // This avoids waking the LoRa module and reading all sensors
    // during the night or in cold weather.
    const uint8_t GATE_SAMPLES = 3;
    uint16_t lux_samples[GATE_SAMPLES];
    int16_t avg_temp_samples[GATE_SAMPLES];

    DHT22Result intr = {0, SENSOR_ERROR_VALUE};
    int16_t s1 = SENSOR_ERROR_VALUE;
    int16_t s2 = SENSOR_ERROR_VALUE;

    for (uint8_t i = 0; i < GATE_SAMPLES; ++i) {
        lux_samples[i] = read_sen0562();
        intr = read_dht22(internal_dht, "Internal DHT22");
        s1 = read_ds18b20_sonde(sonde1, "DS18B20 Sonde 1");
        s2 = read_ds18b20_sonde(sonde2, "DS18B20 Sonde 2");
        avg_temp_samples[i] = average_internal_temp_x10(intr.temperature, s1, s2);

        if (i + 1 < GATE_SAMPLES) {
            delay(200);
        }
    }

    uint16_t lux = median3_u16(lux_samples[0], lux_samples[1], lux_samples[2]);
    int16_t avg_internal_temp = median3_i16(avg_temp_samples[0], avg_temp_samples[1], avg_temp_samples[2]);  // °C × 10

    char check_msg[80];
    snprintf(check_msg, sizeof(check_msg),
             "Climate conditions (median) — lux=%u, int_avg=%.1f C",
             lux,
             avg_internal_temp / 10.0f);
    logInfo(check_msg, "SYSTEM");

    bool first_dormant_uplink = false;
    if (!is_active_period(lux, avg_internal_temp)) {
        ++dormant_streak;
        if (dormant_streak == 1) {
            // First time conditions fall below threshold: send one final payload
            // so the server records the node entering dormant mode.
            logInfo("First dormant boot — sending payload before dormant sleep", "SYSTEM");
            first_dormant_uplink = true;
        } else if (dormant_streak < DORMANT_STREAK_MAX) {
            snprintf(check_msg, sizeof(check_msg),
                     "Dormant mode (lux<%u or temp<%.1f C) — sleeping %lus [streak %d/%d]",
                     LUX_ACTIVITY_THRESHOLD,
                     TEMP_ACTIVITY_THRESHOLD / 10.0f,
                     (unsigned long)DEEP_SLEEP_DORMANT_S,
                     dormant_streak,
                     DORMANT_STREAK_MAX);
            logInfo(check_msg, "SYSTEM");
            digitalWrite(VREG_3V3_PIN, LOW);
            logInfo("Voltage regulator 3.3V OFF", "SLEEP");
            Serial.flush();
            enter_deep_sleep(DEEP_SLEEP_DORMANT_S);
            return;
        } else {
            // Streak limit reached: a sensor may be stuck — force one active cycle
            // to prevent the node from being locked out indefinitely.
            snprintf(check_msg, sizeof(check_msg),
                     "Dormant streak limit (%d) reached — forcing active cycle",
                     DORMANT_STREAK_MAX);
            logWarn(check_msg, "SYSTEM");
            dormant_streak = 0;
        }
    }

    // ── Active mode — full uplink cycle (gate passed, first dormant, or streak override)
    if (!first_dormant_uplink) dormant_streak = 0;
    logInfo(first_dormant_uplink
        ? "Sending final payload before entering dormant mode"
        : "Active period confirmed — starting uplink cycle", "SYSTEM");

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
    shutdown_and_sleep(first_dormant_uplink ? DEEP_SLEEP_DORMANT_S : lora_sleep_duration_s());
}

void loop() {

}