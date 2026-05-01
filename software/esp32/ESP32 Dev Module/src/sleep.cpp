#include "sleep.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_sleep.h"

void print_wakeup_reason() {
#if DEBUG_MODE
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wake-up from external signal with RTC_IO");   break;
        case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wake-up from external signal with RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wake up caused by a timer");                   break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wake up caused by a touchpad");                break;
        default:
            Serial.printf("Wake up not caused by Deep Sleep: %d\n", esp_sleep_get_wakeup_cause());    break;
    }
#endif
}

void sleep_gpio_release() {
    gpio_hold_dis((gpio_num_t)I2C_SDA_PIN);
    gpio_hold_dis((gpio_num_t)I2C_SCL_PIN);
    gpio_hold_dis((gpio_num_t)VREG_3V3_PIN);
    gpio_hold_dis((gpio_num_t)VREG_5V_PIN);
    gpio_hold_dis((gpio_num_t)LORA_TX_PIN);
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

void buzzer_sleep_beep() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
}

void vreg_power_on() {
    pinMode(VREG_3V3_PIN, OUTPUT);
    pinMode(VREG_5V_PIN, OUTPUT);
    digitalWrite(VREG_3V3_PIN, HIGH);
    digitalWrite(VREG_5V_PIN, HIGH);
    logInfo("Voltage regulators ON", "SLEEP");
}

void vreg_power_off() {
    digitalWrite(VREG_3V3_PIN, LOW);
    digitalWrite(VREG_5V_PIN, LOW);
    logInfo("Voltage regulators OFF", "SLEEP");
}

void enter_deep_sleep(uint32_t duration_s) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Entering deep sleep for %lus", (unsigned long)duration_s);
    logInfo(msg, "SLEEP");

    buzzer_sleep_beep();
#if DEBUG_MODE
    Serial.flush();
#endif

    // Hold all controlled pins during deep sleep.
    // LORA_TX_PIN is explicitly driven HIGH before the hold: in dormant mode
    // lora_init() is never called so UART2 is uninitialised, and after
    // sleep_gpio_release() GPIO17 defaults to floating input — which the
    // LoRa-E5 sees as a continuous UART break on its RX line.
    gpio_set_direction((gpio_num_t)LORA_TX_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)LORA_TX_PIN, 1);

    gpio_hold_en((gpio_num_t)VREG_3V3_PIN);
    gpio_hold_en((gpio_num_t)VREG_5V_PIN);
    gpio_hold_en((gpio_num_t)I2C_SDA_PIN);
    gpio_hold_en((gpio_num_t)I2C_SCL_PIN);
    gpio_hold_en((gpio_num_t)LORA_TX_PIN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup((uint64_t)duration_s * uS_TO_S_FACTOR);

    esp_deep_sleep_start();
}
