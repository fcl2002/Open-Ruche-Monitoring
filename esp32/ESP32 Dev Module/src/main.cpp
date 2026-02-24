#include <Arduino.h>
#include "sensors.h"
#include "errors.h"
#include "logger.h"

// #define SEN0562_ADDR 0x1C // ACC I2C address

bool setupDelayDone = false;
unsigned long setupStartTime = 0;

void setup() {
	Serial.begin(115200);
	delay(100);  // Wait for serial to stabilize
	
	// Set log level (can be changed to LOG_DEBUG for verbose output)
	setLogLevel(LOG_INFO);
	
	logInfo("Open Ruche Monitoring System Starting...", "SYSTEM");
    setupStartTime = millis();

    // HX711
    logInfo("Initializing HX711...", "SETUP");
	hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
	delay(200);
	hx711.set_scale();
	hx711.tare();
	delay(200);
	hx711.set_scale(30148.9361702128);

    // DHT22
    logInfo("Initializing DHT22 sensors...", "SETUP");
	external_dht.begin();
	internal_dht.begin();

	// DS18B20 Sondes
	logInfo("Initializing DS18B20 sensors...", "SETUP");
	sondes.begin();
    
    int deviceCount = sondes.getDeviceCount();
    char msg[50];
    snprintf(msg, sizeof(msg), "Found %d DS18B20 device(s)", deviceCount);
    logInfo(msg, "SETUP");
    
    if (deviceCount > 0) {
        sondes.getAddress(sonde1, 0);
    } else {
        logError(ERR_DEVICE_NOT_FOUND, "DS18B20 Sonde 1");
    }
    
    if (deviceCount > 1) {
        sondes.getAddress(sonde2, 1);
    } else if (deviceCount == 1) {
        logError(ERR_DEVICE_NOT_FOUND, "DS18B20 Sonde 2");
    }

    // SEN0562
    logInfo("Initializing I2C for SEN0562...", "SETUP");
    Wire.begin(); // using default SDA/SCL
    
    logInfo("Setup complete. System ready.", "SYSTEM");
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
        logInfo("Starting sensor readings...", "LOOP");
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
        
        int16_t sonde1_read = read_ds18b20_sonde(sonde1, "DS18B20 Sonde 1");
        memcpy(&payload[idx], &sonde1_read, sizeof(int16_t)); 
        idx += 2;

        int16_t sonde2_read = read_ds18b20_sonde(sonde2, "DS18B20 Sonde 2");
        memcpy(&payload[idx], &sonde2_read, sizeof(int16_t)); 
        idx += 2;
        
        int16_t lux_read = read_sen0562();
        memcpy(&payload[idx], &lux_read, sizeof(int16_t)); 
        idx += 2;

        logDebug("Payload ready for transmission", "LOOP");
        for (int i = 0; i < 12; i++) {
            Serial.print(payload[i]); // ou DEC
            Serial.print(" ");
        }
        Serial.println();
        lastRead = millis();
    }
}