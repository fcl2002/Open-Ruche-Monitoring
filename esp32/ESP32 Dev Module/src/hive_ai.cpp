#include "hive_ai.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>

// UART1 — microphone AI board (hardware UART)
static HardwareSerial aiSerial(AI_UART_NUM);
// SoftwareSerial — camera AI board (no HW UART left: UART0=USB, UART1=mic, UART2=LoRa)
static SoftwareSerial camSerial(CAM_RX_PIN, -1);  // RX only

void hive_ai_init(void) {
    aiSerial.begin(AI_BAUD, SERIAL_8N1, AI_RX_PIN, -1);
    logInfo("AI UART ready", "AI");

    camSerial.begin(CAM_BAUD);
    logInfo("Camera UART ready", "CAM");
}

uint8_t read_microphone(void) {
    unsigned long deadline = millis() + AI_READ_TIMEOUT_MS;
    while (!aiSerial.available() && millis() < deadline) {
        delay(10);
    }

    if (!aiSerial.available()) return 0;

    return (uint8_t)aiSerial.read();
}

uint8_t read_camera(void) {
    unsigned long deadline = millis() + CAM_READ_TIMEOUT_MS;
    while (!camSerial.available() && millis() < deadline) {
        delay(10);
    }

    if (!camSerial.available()) return 0;

    return (uint8_t)camSerial.read();
}
