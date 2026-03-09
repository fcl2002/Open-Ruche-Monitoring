#include "sleep.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include "driver/gpio.h"
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
}

void enter_deep_sleep() {
    char msg[48];
    snprintf(msg, sizeof(msg), "Entering deep sleep. Waking up in %ds...", DEEP_SLEEP_DURATION_S);
    logInfo(msg, "SLEEP");
    Serial.flush();

    gpio_hold_en((gpio_num_t)I2C_SDA_PIN);
    gpio_hold_en((gpio_num_t)I2C_SCL_PIN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup((uint64_t)DEEP_SLEEP_DURATION_S * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}
