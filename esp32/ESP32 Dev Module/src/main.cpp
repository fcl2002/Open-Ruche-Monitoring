#include <Arduino.h>
#include "sensors.h"

// #include <Wire.h>
// #define SEN0562_ADDR 0x23 // LUX I2C address
// #define SEN0562_ADDR 0x1C // ACC I2C address

bool setupDelayDone = false;
unsigned long setupStartTime = 0;

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

	// DS18B20 Sondes
	sondes.begin();
    
    if (sondes.getDeviceCount() > 0)
        sondes.getAddress(sonde1, 0);
    if (sondes.getDeviceCount() > 1)
        sondes.getAddress(sonde2, 1);

    //Wire.begin(); // Use default SDA/SCL
}

void loop() {
    if (!setupDelayDone) {
        if (millis() - setupStartTime >= TIME_SETUP)
            setupDelayDone = true;
        return;
    }

    // Read DHT22 every 10 minutes
    if (millis() - lastRead >= TIME_TO_READ) {
        // read_hx711(); // il faut vérifier encore
        read_dht22(external_dht, "External DHT22");
        read_dht22(internal_dht, "Internal DHT22");
        read_ds18b20_sonde(sonde1);
        read_ds18b20_sonde(sonde2);
        Serial.println();
        lastRead = millis();
    }
}


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