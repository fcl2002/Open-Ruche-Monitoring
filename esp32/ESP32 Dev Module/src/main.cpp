#include <Arduino.h>
#include "sensors.h"

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

    // SEN0562
    Wire.begin(); // using default SDA/SCL
}

uint8_t payload[12];

void loop() {
    if (!setupDelayDone) {
        if (millis() - setupStartTime >= TIME_SETUP)
            setupDelayDone = true;
        return;
    }

    
    // Read DHT22 every 10 minutes
    if (millis() - lastRead >= TIME_TO_READ) {
        int idx = 0;

        // read_hx711(); // il faut vérifier encore

        DHT22Result ext_dht22 = read_dht22(external_dht, "External DHT22");
        payload[idx++] = ext_dht22.humidity;
        memcpy(&payload[idx], &ext_dht22.temperature, sizeof(int16_t)); 
        idx += 2;
        
        DHT22Result int_dht22 = read_dht22(internal_dht, "Internal DHT22");
        payload[idx++] = int_dht22.humidity;
        memcpy(&payload[idx], &int_dht22.temperature, sizeof(int16_t)); 
        idx += 2;
        
        int16_t sonde1_read = read_ds18b20_sonde(sonde1);
        memcpy(&payload[idx], &sonde1_read, sizeof(int16_t)); 
        idx += 2;

        int16_t sonde2_read = read_ds18b20_sonde(sonde2);
        memcpy(&payload[idx], &sonde2_read, sizeof(int16_t)); 
        idx += 2;
        
        int16_t lux_read = read_sen0562();
        memcpy(&payload[idx], &lux_read, sizeof(int16_t)); 
        idx += 2;

        for (int i = 0; i < 12; i++) {
            Serial.print(payload[i], HEX); // ou DEC
            Serial.print(" ");
        }
        Serial.println();
        lastRead = millis();
    }
}