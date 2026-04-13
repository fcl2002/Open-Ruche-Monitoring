#include "boards.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>

// UART1 — microphone AI board (hardware UART)
static HardwareSerial micSerial(AI_UART_NUM);
// SoftwareSerial — camera AI board (no HW UART left: UART0=USB, UART1=mic, UART2=LoRa)
static SoftwareSerial camSerial(CAM_RX_PIN, -1);  // RX only

void boards_init(void) {
    micSerial.begin(AI_BAUD, SERIAL_8N1, AI_RX_PIN, -1);
    logInfo("Microphone UART ready", "MIC");

    camSerial.begin(CAM_BAUD);
    logInfo("Camera UART ready", "CAM");
}

uint8_t read_audio(void) {
    unsigned long deadline = millis() + AI_READ_TIMEOUT_MS;
    while (!micSerial.available() && millis() < deadline) {
        delay(10);
    }

    if (!micSerial.available()) return 0;
    micSerial.flush();
    return (uint8_t)micSerial.read();
}

uint8_t read_camera(void) {
    unsigned long deadline = millis() + CAM_READ_TIMEOUT_MS;
    while (!camSerial.available() && millis() < deadline) {
        delay(10);
    }

    if (!camSerial.available()) return 0;
    camSerial.flush();
    return (uint8_t)camSerial.read();
}
