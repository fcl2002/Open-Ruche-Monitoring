#include <Arduino.h>
#include "HX711.h"

#define HX711_DOUT 32 // DT pin
#define HX711_SCK 33  // SCK pin

HX711 hx711;
bool start = false;
long previousMillis = 0;
const long interval = 5000;

void setup() {
	Serial.begin(115200);
	hx711.begin(HX711_DOUT, HX711_SCK);
	delay(200);
	
	hx711.set_scale();
	hx711.tare();
	delay(200);

	Serial.println("HX711 Calibration");
	hx711.set_scale(30148.9361702128);
	delay(5000);
	Serial.println("Type 'start' to begin weighing every 5 seconds, 'stop' to stop.");
}

void loop() {
	if (Serial.available()) {
		String comando = Serial.readStringUntil('\n');
        comando.trim();
        if (comando == "start") {
            start = true;
        }
        if (comando == "stop") {
            start = false;
        }
    }

	if (start && (millis() - previousMillis >= interval)) {
		previousMillis = millis();
		
		hx711.power_up();
		float units = hx711.get_units(10);
		Serial.print("Weight: ");
		Serial.print(units, 2);
		Serial.println(" kg");
		hx711.power_down();
	}
}