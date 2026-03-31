#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "errors.h"
#include "logger.h"
#include "payload.h"
#include "sleep.h"
#include "lora.h"
#include "hive_ai.h"

RTC_DATA_ATTR int bootCount = 0;

// ── Activity gate ─────────────────────────────────────────────
// Returns true when environmental conditions justify a full uplink
// cycle. Both conditions must be met simultaneously:
//   • lux  >= LUX_ACTIVITY_THRESHOLD  (daylight, bees are active)
//   • temp >= TEMP_ACTIVITY_THRESHOLD (above minimum foraging temp)
// If either is below threshold the system is in dormant mode and
// goes back to deep sleep without joining LoRa or reading all sensors.
static bool is_active_period(uint16_t lux, int16_t ext_temp_x10) {
    return (lux      >= LUX_ACTIVITY_THRESHOLD) &&
           (ext_temp_x10 >= TEMP_ACTIVITY_THRESHOLD);
}

static void shutdown_and_sleep(uint32_t duration_s) {
    logInfo("Preparing LoRa module for sleep...", "SYSTEM");
    lora_sleep();
    Serial.flush();
    enter_deep_sleep(duration_s);
}

void setup() {
    Serial.begin(115200);
    delay(100);

    ++bootCount;
    Serial.println("\n----------------------");
    Serial.println(String(bootCount) + "th Boot");

    sleep_gpio_release();
    vreg_power_on();
    Serial.println("[DEBUG] VREGs ON — waiting 750ms for sensors to stabilize...");
    delay(750);  // DS18B20 + I2C sensors need time after power-on

    setLogLevel(LOG_INFO);
    print_wakeup_reason();

    logInfo("Open Ruche Monitoring System Starting...", "SYSTEM");

    Serial.println("[DEBUG] Starting sensors_init...");
    sensors_init();
    Serial.println("[DEBUG] sensors_init done");
    hive_ai_init();

    // ── Quick environmental check ──────────────────────────────
    // Read only luminosity and external temperature — cheap reads
    // that decide whether conditions are suitable for a full cycle.
    // This avoids waking the LoRa module and reading all sensors
    // during the night or in cold weather.
    uint16_t    lux      = read_sen0562();
    DHT22Result ext      = read_dht22(external_dht, "External DHT22");
    int16_t     ext_temp = ext.temperature;  // °C × 10

    char check_msg[80];
    snprintf(check_msg, sizeof(check_msg),
             "Quick check — lux=%u, ext_temp=%.1f C",
             lux, ext_temp / 10.0f);
    logInfo(check_msg, "SYSTEM");

    // if (!is_active_period(lux, ext_temp)) {
    //     snprintf(check_msg, sizeof(check_msg),
    //              "Dormant mode (lux<%u or temp<%.1f C) — sleeping %lus",
    //              LUX_ACTIVITY_THRESHOLD,
    //              TEMP_ACTIVITY_THRESHOLD / 10.0f,
    //              (unsigned long)DEEP_SLEEP_DORMANT_S);
    //     logInfo(check_msg, "SYSTEM");
    //     Serial.flush();
    //     enter_deep_sleep(DEEP_SLEEP_DORMANT_S);
    //     return;
    // }

    // ── Active mode — full uplink cycle ───────────────────────
    logInfo("Active period confirmed — starting uplink cycle", "SYSTEM");

    lora_init();

    // Wait for OTAA join — the LoRa-E5 responds quickly if session is still active.
    logInfo("Waiting for LoRa network join...", "SYSTEM");
    const unsigned long JOIN_TIMEOUT_MS = 15000;
    unsigned long deadline = millis() + JOIN_TIMEOUT_MS;
    while (!lora_is_joined() && millis() < deadline) {
        lora_tick();
        delay(50);
    }

    // if (!lora_is_joined()) {
    //     logError(ERR_TIMEOUT, "LoRa join");
    //     shutdown_and_sleep(DEEP_SLEEP_DURATION_S);
    //     return;
    // }

    logInfo("Reading sensors...", "SYSTEM");

    const LoRaCalibration& cal = lora_calibration();
    SensorPayload payload = {0};

    // External DHT22 — reuse the reading already taken in the quick check
    payload.ext_humidity    = (int8_t)constrain(ext.humidity + cal.humOffset, 0, 100);
    payload.ext_temperature = (ext_temp != SENSOR_ERROR_VALUE)
                                  ? ext_temp + cal.tempOffset
                                  : (int16_t)SENSOR_ERROR_VALUE;

    // Internal DHT22 — apply humidity and temperature offsets
    DHT22Result intr = read_dht22(internal_dht, "Internal DHT22");
    payload.int_humidity    = (int8_t)constrain(intr.humidity + cal.humOffset, 0, 100);
    payload.int_temperature = (intr.temperature != SENSOR_ERROR_VALUE)
                                  ? intr.temperature + cal.tempOffset
                                  : (int16_t)SENSOR_ERROR_VALUE;

    // DS18B20 sondes — apply temperature offset
    int16_t s1 = read_ds18b20_sonde(sonde1, "DS18B20 Sonde 1");
    payload.sonde1_temperature = (s1 != SENSOR_ERROR_VALUE)
                                     ? s1 + cal.tempOffset
                                     : (int16_t)SENSOR_ERROR_VALUE;

    int16_t s2 = read_ds18b20_sonde(sonde2, "DS18B20 Sonde 2");
    payload.sonde2_temperature = (s2 != SENSOR_ERROR_VALUE)
                                     ? s2 + cal.tempOffset
                                     : (int16_t)SENSOR_ERROR_VALUE;

    // Luminosity — reuse the reading already taken in the quick check
    payload.lux = lux;

    // Weight: net weight after tare (g*100)
    payload.weight = read_hx711();

    // Microphone AI classifier via UART1
    payload.hive_status = read_microphone();

    // Camera AI classifier via SoftwareSerial (GPIO25)
    payload.camera_status = read_camera();

    // Battery voltage via ADC (GPIO35)
    payload.battery_v = read_battery_v();
    Serial.printf("Battery: %d%%\n", payload.battery_v);

    Serial.printf("Sonde 1: %.1f C\n", payload.sonde1_temperature / 10.0f);
    Serial.printf("Sonde 2: %.1f C\n", payload.sonde2_temperature / 10.0f);

    lora_send(payload);

    // Print hive_ai value to terminal
    Serial.print("hive_ai value: ");
    Serial.println(payload.hive_status);

    Serial.print("camera value: ");
    Serial.println(payload.camera_status);
   
    Serial.printf("Lux: %u\n", payload.lux);

    // Listen for Class A downlink windows before sleeping
    deadline = millis() + 5000;
    while (millis() < deadline) {
        lora_tick();
        delay(50);
    }
    shutdown_and_sleep(DEEP_SLEEP_DURATION_S);
}

void loop() {

}

// #include <OneWire.h>
// #include <DallasTemperature.h>

// OneWire testWire(ONE_WIRE_BUS);
// DallasTemperature testSondes(&testWire);

// void setup() {
//     Serial.begin(115200);
//     delay(200);
//     testSondes.begin();
//     Serial.printf("DS18B20 devices found: %d\n", testSondes.getDeviceCount());
// }

// void loop() {
//     testSondes.requestTemperatures();
//     int n = testSondes.getDeviceCount();
//     for (int i = 0; i < n; i++) {
//         Serial.printf("Sonde [%d]: %.1f C\n", i, testSondes.getTempCByIndex(i));
//     }
//     if (n == 0) Serial.println("No devices found — check wiring");
//     Serial.println("---");
//     delay(2000);
// }