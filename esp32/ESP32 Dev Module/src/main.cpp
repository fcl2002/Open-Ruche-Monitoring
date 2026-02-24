#include "DHT.h"
#include "HX711.h"
#include <Arduino.h>
#include "sensors.h"

// #include "Wire.h"
// #define SEN0562_ADDR 0x23

// #include <OneWire.h>
// #include <DallasTemperature.h>

// #define ONE_WIRE_BUS 4

// OneWire oneWire(ONE_WIRE_BUS);
// DallasTemperature sensors(&oneWire);

// DeviceAddress sensor1, sensor2;

unsigned long setupStartTime = 0;
bool setupDelayDone = false;

// void printAddress(DeviceAddress deviceAddress) {
//     for (uint8_t i = 0; i < 8; i++) {
//         if (deviceAddress[i] < 16) Serial.print("0");
//         Serial.print(deviceAddress[i], HEX);
//     }
// }

void setup() {
	Serial.begin(115200);
    setupStartTime = millis();

    // HX711
	hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
	delay(200);
	hx711.set_scale();
	hx711.tare();
	delay(200);
	hx711.set_scale(30148.9361702128);

    // DHT22
	external_dht.begin();
	internal_dht.begin();

	//
	// sensors.begin();

    // Serial.print("Found ");
    // Serial.print(sensors.getDeviceCount());
    // Serial.println(" devices.");

    // if (sensors.getDeviceCount() > 0) {
    //     sensors.getAddress(sensor1, 0);
    //     Serial.print("Sensor 1 Address: ");
    //     printAddress(sensor1);
    //     Serial.println();
    // }
    // if (sensors.getDeviceCount() > 1) {
    //     sensors.getAddress(sensor2, 1);
    //     Serial.print("Sensor 2 Address: ");
    //     printAddress(sensor2);
    //     Serial.println();
    // }

	// Wire.begin();
}

void loop() {
    if (!setupDelayDone) {
        if (millis() - setupStartTime >= TIME_SETUP)
            setupDelayDone = true;
        return;
    }

    // Read DHT22 every 10 minutes
    if (millis() - lastReadDht >= TIME_DHT22) {
        read_dht22(external_dht, "External DHT22");
        read_dht22(internal_dht, "Internal DHT22");
        Serial.println();
        lastReadDht = millis();
    }
}

// void read_DS18B20_sondes() {
//     sensors.requestTemperatures();

//     if (sensors.isConnected(sensor1)) {
//         float temp1 = sensors.getTempC(sensor1);
//         Serial.print("Sensor 1 [");
//         printAddress(sensor1);
//         Serial.print("]: ");
//         Serial.print(temp1);
//         Serial.println(" °C");
//     }

//     if (sensors.isConnected(sensor2)) {
//         float temp2 = sensors.getTempC(sensor2);
//         Serial.print("Sensor 2 [");
//         printAddress(sensor2);
//         Serial.print("]: ");
//         Serial.print(temp2);
//         Serial.println(" °C");
//     }

//     delay(2000);
// }

// void loop() {
    
    // read_external_dht22();

	// if (Serial.available()) {
	// 	String command = Serial.readStringUntil('\n');
    //     command.trim();
    //     if (command == "start") {
    //         start = true;
    //     }
    //     if (command == "stop") {
    //         start = false;
    //     }
    // }

	// read_DS18B20_sondes();
	// if (start) {
	// 	// if (millis() - previousMillis >= interval) {
	// 	// 	read_hx711();
	// 	// }
	// 	// if (millis() - previousMillis >= interval) {
	// 	// 	read_external_dht22();
	// 	// }
	// 	// read_external_dht22();
	// }
// }

// #include <Wire.h>
// #define SEN0562_ADDR 0x23 // I2C address

// void setup() {
//     Serial.begin(115200);
//     Wire.begin(); // Use default SDA/SCL
// }

// uint8_t readReg(uint8_t reg, uint8_t* pBuf, size_t size) {
//     Wire.beginTransmission(SEN0562_ADDR);
//     Wire.write(reg);
//     if (Wire.endTransmission() != 0) return 0;
//     delay(20);
//     Wire.requestFrom(SEN0562_ADDR, (uint8_t)size);
//     for (uint16_t i = 0; i < size; i++) {
//         pBuf[i] = Wire.read();
//     }
//     return size;
// }

// void loop() {
//     uint8_t buf[2] = {0};
//     readReg(0x10, buf, 2); // Register 0x10
//     uint16_t data = buf[0] << 8 | buf[1];
//     float lux = ((float)data) / 1.2;
//     Serial.print("LUX: ");
//     Serial.print(lux);
//     Serial.println(" lx");
//     delay(500);
// }