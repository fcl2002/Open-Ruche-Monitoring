#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "errors.h"
#include "logger.h"
#include "payload.h"
#include "sleep.h"
#include "lora.h"

RTC_DATA_ATTR int bootCount = 0;

void setup() {
    Serial.begin(115200);
    delay(100);

    ++bootCount;
    Serial.println("\n----------------------");
    Serial.println(String(bootCount) + "th Boot");

    sleep_gpio_release();

    setLogLevel(LOG_INFO);
    print_wakeup_reason();

    logInfo("Open Ruche Monitoring System Starting...", "SYSTEM");

    sensors_init();
    lora_init();
    pinMode(35, INPUT);

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
        Serial.flush();
        enter_deep_sleep();
        return;
    }

    logInfo("Reading sensors...", "SYSTEM");

    const LoRaCalibration& cal = lora_calibration();
    SensorPayload payload = {0};

    // External DHT22 — apply humidity and temperature offsets
    DHT22Result ext = read_dht22(external_dht, "External DHT22");
    payload.ext_humidity    = (int8_t)constrain(ext.humidity + cal.humOffset, 0, 100);
    payload.ext_temperature = (ext.temperature != SENSOR_ERROR_VALUE)
                                  ? ext.temperature + cal.tempOffset
                                  : (int16_t)SENSOR_ERROR_VALUE;
    
    Serial.print("DHT Externe\n");
    Serial.println(payload.ext_humidity);
    Serial.println(payload.ext_temperature);

    // Internal DHT22 — apply humidity and temperature offsets
    DHT22Result intr = read_dht22(internal_dht, "Internal DHT22");
    payload.int_humidity    = (int8_t)constrain(intr.humidity + cal.humOffset, 0, 100);
    payload.int_temperature = (intr.temperature != SENSOR_ERROR_VALUE)
                                  ? intr.temperature + cal.tempOffset
                                  : (int16_t)SENSOR_ERROR_VALUE;

    Serial.print("DHT Interne\n");
    Serial.println(payload.int_humidity);
    Serial.println(payload.int_temperature);

    // DS18B20 sondes — apply temperature offset
    int16_t s1 = read_ds18b20_sonde(sonde1, "DS18B20 Sonde 1");
    payload.sonde1_temperature = (s1 != SENSOR_ERROR_VALUE)
                                     ? s1 + cal.tempOffset
                                     : (int16_t)SENSOR_ERROR_VALUE;
    
    Serial.print("Sonde 1\n");
    Serial.println(payload.sonde1_temperature);

    int16_t s2 = read_ds18b20_sonde(sonde2, "DS18B20 Sonde 2");
    payload.sonde2_temperature = (s2 != SENSOR_ERROR_VALUE)
                                     ? s2 + cal.tempOffset
                                     : (int16_t)SENSOR_ERROR_VALUE;

    Serial.print("Sonde 2\n");
    Serial.println(payload.sonde2_temperature);

    // Luminosity
    payload.lux = read_sen0562();
        
    Serial.print("Lux\n");
    Serial.println(payload.lux);

    // Accelerometer
    AccelResult accel = read_mma8451();
    // payload.accel_x = accel.x;
    payload.accel_x = 10;
    payload.accel_y = accel.y;
    payload.accel_z = accel.z;

    Serial.print("Acc (x/y/z)\n");
    Serial.println(payload.accel_x);
    Serial.println(payload.accel_y);
    Serial.println(payload.accel_z);

    // Weight: net weight after tare (g*100).
    payload.weight = read_hx711();

    Serial.print("Poids\n");
    Serial.println(payload.weight);

    // Battery

    lora_send(payload);

    // Listen for Class A downlink windows before sleeping
    deadline = millis() + 5000;
    while (millis() < deadline) {
        lora_tick();
        delay(50);
    }

    Serial.flush();
    enter_deep_sleep();
}

void loop() {

}
