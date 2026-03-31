/**
 *
 * LoRa-E5 communication module for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#include "lora.h"
#include "config.h"
#include "sensors.h"
#include "logger.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <Preferences.h>

static HardwareSerial loraSerial(LORA_SERIAL_NUM);
static Preferences    prefs;

static LoRaCalibration cal;
static bool            joined     = false;
static unsigned long   lastSendMs = 0;
static bool            joinLogged  = false;

// ─── NVS helpers ──────────────────────────────────────────────────────────────

static void load_calibration() {
    prefs.begin("nbee", false);
    cal.sendInterval = prefs.getUInt("interval",   LORA_DEFAULT_INTERVAL_MS);
    cal.tempOffset   = (int16_t)prefs.getInt("tempOffset", 0);
    cal.humOffset    = (int8_t) prefs.getInt("humOffset",  0);
    cal.tareWeight   = prefs.getInt("tareWeight", 0);
    prefs.end();
}

static void save_interval() {
    prefs.begin("nbee", false); prefs.putUInt ("interval",   cal.sendInterval);  prefs.end();
}
static void save_temp() {
    prefs.begin("nbee", false); prefs.putInt("tempOffset", cal.tempOffset);      prefs.end();
}
static void save_hum() {
    prefs.begin("nbee", false); prefs.putInt  ("humOffset",  cal.humOffset);     prefs.end();
}
static void save_tare() {
    prefs.begin("nbee", false); prefs.putInt  ("tareWeight", cal.tareWeight);    prefs.end();
}

// ─── AT command helper ────────────────────────────────────────────────────────

static void send_at(const String& cmd) {
    if (cmd.startsWith("AT+JOIN")) {
        logInfo("TX: Join request sent", "LORA");
    } else if (cmd.startsWith("AT+CMSGHEX")) {
        logInfo("TX: Uplink frame sent", "LORA");
    } else if (cmd.startsWith("AT+MODE")) {
        logInfo("TX: LoRaWAN mode configured", "LORA");
    } else if (cmd.startsWith("AT+SLEEP")) {
        logInfo("TX: Sleep command sent", "LORA");
    } else {
        logInfo("TX: Command sent", "LORA");
    }
    loraSerial.println(cmd);
}

// ─── Downlink parser ──────────────────────────────────────────────────────────

static void handle_downlink(const String& line) {
    logInfo("Downlink received", "LORA");

    int qs = line.indexOf('"', line.indexOf("RX:"));
    int qe = line.indexOf('"', qs + 1);
    if (qs < 0 || qe <= qs) return;

    String hex      = line.substring(qs + 1, qe);
    if (hex.length() < 4) return;

    String cmdType  = hex.substring(0, 2);
    String valueHex = hex.substring(2, 4);
    int    valueInt = (int)strtol(valueHex.c_str(), nullptr, 16);

    char msg[64];

    if (cmdType == "01") {                              // Change uplink interval (minutes)
        if (valueInt >= 1 && valueInt <= 60) {
            cal.sendInterval = (uint32_t)valueInt * 60 * 1000UL;
            save_interval();
            snprintf(msg, sizeof(msg), "Interval set to %d min", valueInt);
            logInfo(msg, "LORA");
        }
    } else if (cmdType == "02") {                       // Temperature offset (int8, tenths of °C)
        cal.tempOffset = (int16_t)(int8_t)valueInt;      // already in tenths of °C
        save_temp();
        snprintf(msg, sizeof(msg), "Temp offset set to %.1f C", cal.tempOffset / 10.0f);
        logInfo(msg, "LORA");
    } else if (cmdType == "03") {                       // Tare
        if (valueInt == 1) {
            cal.tareWeight = (int32_t)read_hx711();
            save_tare();
            logInfo("Tare applied", "LORA");
        } else if (valueInt == 0) {
            cal.tareWeight = 0;
            save_tare();
            logInfo("Tare reset", "LORA");
        }
    } else if (cmdType == "04") {                       // Humidity offset (%, signed)
        cal.humOffset = (int8_t)valueInt;
        save_hum();
        snprintf(msg, sizeof(msg), "Humidity offset set to %d%%", (int)cal.humOffset);
        logInfo(msg, "LORA");
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

void lora_init() {
    load_calibration();

    logInfo("LoRa init started", "LORA");

    loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
    vTaskDelay(1000);
    logInfo("Serial link ready", "LORA");

    send_at("AT+MODE=LWOTAA"); vTaskDelay(500);

    logInfo("Join procedure started", "LORA");
    send_at("AT+JOIN");
}

void lora_tick() {
    // Read all incoming lines from the LoRa-E5
    while (loraSerial.available()) {
        String line = loraSerial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        if (!joinLogged && (line.indexOf("Join") >= 0 || line.indexOf("JOIN") >= 0)) {
            logInfo("RX: Join status update", "LORA");
            joinLogged = true;
        }

        if (line.indexOf("ERROR") >= 0 || line.indexOf("Error") >= 0 || line.indexOf("error") >= 0) {
            logWarn("RX: Module reported an error", "LORA");
        }

        if (!joined && (line.indexOf("Network joined") >= 0 ||
                        line.indexOf("Already joined") >= 0)) {
            joined = true;
            lastSendMs = millis() - cal.sendInterval; // trigger uplink immediately
            logInfo("Join successful. Uplink ready", "LORA");
        }

        if (line.indexOf("RX:") >= 0) {
            handle_downlink(line);
        }
    }

    // Forward Serial-monitor input to the LoRa-E5 (manual AT commands during debug)
    while (Serial.available()) {
        loraSerial.write(Serial.read());
    }
}

bool lora_should_send() {
    return joined && (millis() - lastSendMs >= cal.sendInterval);
}

void lora_send(const SensorPayload& payload) {
    lastSendMs = millis();

    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&payload);
    String hexPayload;
    hexPayload.reserve(sizeof(SensorPayload) * 2 + 1);
    for (size_t i = 0; i < sizeof(SensorPayload); i++) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", raw[i]);
        hexPayload += buf;
    }

    logInfo(("Uplink prepared (" + String(sizeof(SensorPayload)) + " bytes)").c_str(), "LORA");
    send_at("AT+CMSGHEX=\"" + hexPayload + "\"");
}

void lora_sleep() {
    logInfo("Sleep procedure started", "LORA");
    send_at("AT+SLEEP");
    delay(50);
}

const LoRaCalibration& lora_calibration() {
    return cal;
}

bool lora_is_joined() {
    return joined;
}
