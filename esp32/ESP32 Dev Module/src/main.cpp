#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "errors.h"
#include "logger.h"
#include "payload.h"
#include "sleep.h"

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
    init_sensors();
    logInfo("Setup complete. System ready.", "SYSTEM");
}

void loop() {
    logInfo("Starting sensor readings...", "LOOP");

    SensorPayload payload = {};

    DHT22Result ext_dht22 = read_dht22(external_dht, "External DHT22");
    payload.ext_humidity = ext_dht22.humidity;
    payload.ext_temperature = ext_dht22.temperature;

    DHT22Result int_dht22 = read_dht22(internal_dht, "Internal DHT22");
    payload.int_humidity = int_dht22.humidity;
    payload.int_temperature = int_dht22.temperature;

    payload.sonde1_temperature = read_ds18b20_sonde(sonde1, "DS18B20 Sonde 1");
    payload.sonde2_temperature = read_ds18b20_sonde(sonde2, "DS18B20 Sonde 2");

    payload.lux = read_sen0562();

    AccelResult accel = read_mma8451();
    payload.accel_x = accel.x;
    payload.accel_y = accel.y;
    payload.accel_z = accel.z;

    payload.weight              = read_hx711();

    // TODO: transmit (uint8_t*)&payload, sizeof(SensorPayload)

    Serial.flush();
    enter_deep_sleep();
}
