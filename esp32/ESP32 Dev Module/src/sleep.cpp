#include "sleep.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_sleep.h"

void print_wakeup_reason() {
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wake-up from external signal with RTC_IO");   break;
        case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wake-up from external signal with RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wake up caused by a timer");                   break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wake up caused by a touchpad");                break;
        default:
            Serial.printf("Wake up not caused by Deep Sleep: %d\n", esp_sleep_get_wakeup_cause());    break;
    }
}

void sleep_gpio_release() {
    gpio_hold_dis((gpio_num_t)I2C_SDA_PIN);
    gpio_hold_dis((gpio_num_t)I2C_SCL_PIN);
    gpio_hold_dis((gpio_num_t)VREG1_PIN);
    gpio_hold_dis((gpio_num_t)VREG2_PIN);
}

void buzzer_boot_beep() {
    pinMode(BUZZER_PIN, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}

void vreg_power_on() {
    pinMode(VREG1_PIN, OUTPUT);
    pinMode(VREG2_PIN, OUTPUT);
    digitalWrite(VREG1_PIN, HIGH);
    digitalWrite(VREG2_PIN, HIGH);
    logInfo("Voltage regulators ON", "SLEEP");
}

void enter_deep_sleep(uint32_t duration_s) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Entering deep sleep for %lus", (unsigned long)duration_s);
    logInfo(msg, "SLEEP");

    Serial.flush();

    // Power off voltage regulators before sleep
    digitalWrite(VREG1_PIN, LOW);
    digitalWrite(VREG2_PIN, LOW);
    logInfo("Voltage regulators OFF", "SLEEP");

    // Hold all controlled pins during deep sleep
    gpio_hold_en((gpio_num_t)VREG1_PIN);
    gpio_hold_en((gpio_num_t)VREG2_PIN);
    gpio_hold_en((gpio_num_t)I2C_SDA_PIN);
    gpio_hold_en((gpio_num_t)I2C_SCL_PIN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup((uint64_t)duration_s * uS_TO_S_FACTOR);

    esp_deep_sleep_start();
}
