#include <Arduino.h>
#include "sensors.h"

HX711 hx711;

DHT external_dht(EXTERNAL_DTH22_DATA_PIN, DHT22);
DHT internal_dht(INTERNAL_DTH22_DATA_PIN, DHT22);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sondes(&oneWire);
DeviceAddress sonde1 = {0x28, 0x4C, 0xB5, 0x68, 0x10, 0x00, 0x00, 0x4D}; //fil orange
DeviceAddress sonde2 = {0x28, 0x33, 0xBA, 0x69, 0x10, 0x00, 0x00, 0x11};

void sensors_init(void) {
    // Initialize I2C bus for all I2C sensors (SEN0562, etc.).
#if DEBUG_MODE
    Serial.println("[DEBUG] Wire.begin...");
#endif
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // I2C bus scan
#if DEBUG_MODE
    Serial.println("[DEBUG] I2C scan:");
#endif
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
#if DEBUG_MODE
            Serial.printf("[DEBUG]   device at 0x%02X\n", addr);
#endif
            found++;
        }
    }
#if DEBUG_MODE
    if (found == 0) Serial.println("[DEBUG]   no I2C devices found");
#endif

#if DEBUG_MODE
    Serial.println("[DEBUG] hx711_init...");
#endif
    hx711_init();
#if DEBUG_MODE
    Serial.println("[DEBUG] DHT begin...");
#endif
	external_dht.begin();
	internal_dht.begin();
    logInfo("DHT22 sensors initialized", "SETUP");
#if DEBUG_MODE
    Serial.println("[DEBUG] sondes_init...");
#endif
    sondes_init();
}

void hx711_init(void) {
    hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
	vTaskDelay(200);
	hx711.set_scale(HX711_SCALE);
	hx711.set_offset(HX711_OFFSET);
	vTaskDelay(200);
    logInfo("HX711 initialized: scale=30148, offset=134750", "SETUP");
}

void sondes_init(void) {
    sondes.begin();
    int deviceCount = sondes.getDeviceCount();
#if DEBUG_MODE
    Serial.printf("[DEBUG] DS18B20 devices found on bus: %d\n", deviceCount);
#endif

    if (deviceCount > 0) {
        sondes.getAddress(sonde1, 0);
#if DEBUG_MODE
        Serial.printf("[DEBUG] Sonde 1 addr: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
            sonde1[0],sonde1[1],sonde1[2],sonde1[3],sonde1[4],sonde1[5],sonde1[6],sonde1[7]);
#endif
    } else {
        logError(ERR_DEVICE_NOT_FOUND, "DS18B20 Sonde 1");
    }

    if (deviceCount > 1) {
        sondes.getAddress(sonde2, 1);
#if DEBUG_MODE
        Serial.printf("[DEBUG] Sonde 2 addr: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
            sonde2[0],sonde2[1],sonde2[2],sonde2[3],sonde2[4],sonde2[5],sonde2[6],sonde2[7]);
#endif
    } else if (deviceCount == 1) {
        logError(ERR_DEVICE_NOT_FOUND, "DS18B20 Sonde 2");
    }

    if (deviceCount > 0)
        logInfo("DS18B20 sensors initialized", "SETUP");
}

uint16_t read_hx711() {
	hx711.power_up();
	float units = hx711.get_units(10);
	hx711.power_down();
	return (uint16_t)(units*100);
}

DHT22Result read_dht22(DHT& dht, const char* sensorName){
	DHT22Result result;
	float humidity = dht.readHumidity();
	float temperature = dht.readTemperature();
	
	if (isnan(humidity) || isnan(temperature)) {
		logError(ERR_SENSOR_READ_FAILED, sensorName);
		result.humidity = 0;
		result.temperature = SENSOR_ERROR_VALUE;
		return result;
	}
	
	result.humidity = (int8_t)humidity;
	result.temperature = (int16_t)(temperature * 10);
	return result;
}

int16_t read_ds18b20_sonde(DeviceAddress sensorAddr, const char* sensorName) {
    sondes.requestTemperatures();
    if (sondes.isConnected(sensorAddr)) {
        float temp = sondes.getTempC(sensorAddr);
        if (temp == DEVICE_DISCONNECTED_C) {
            logError(ERR_SENSOR_READ_FAILED, sensorName);
            return SENSOR_ERROR_VALUE;
        }
        return (int16_t)(temp * 10);
    } else {
        logError(ERR_DEVICE_NOT_FOUND, sensorName);
        return SENSOR_ERROR_VALUE;
    }
}

uint8_t read_battery_v() {
    pinMode(BATTERY_ADC_PIN, INPUT);
    // Average 10 samples to reduce ADC noise
    int sum = 0;
    for (int n = 0; n < 10; n++) sum += analogRead(BATTERY_ADC_PIN);
    float raw    = sum / 10.0f;
    float batt_v = 1.435f * (raw / 4095.0f) * 3.3f;
    // 3.3 V = 0%, 4.2 V = 100%
    float pct = (batt_v - 3.3f) / (4.2f - 3.3f) * 100.0f;
    if(pct > 100.0f) pct = 100.0f;
    if(pct < 0.0f) pct = 0.0f;
    return (uint8_t)pct;
}

uint16_t read_sen0562() {
    // Re-send measurement command after deep sleep wakeup
    Wire.beginTransmission(SEN0562_ADDR);
    Wire.write(0x10); // Continuously H-Resolution Mode
    if (Wire.endTransmission() != 0) {
        logError(ERR_I2C_COMMUNICATION_FAILED, "SEN0562");
        return SENSOR_LUX_ERROR_VALUE;
    }

    delay(180); // BH1750 precisa de até 180ms para medir

    uint8_t buf[2] = {0};
    Wire.requestFrom(SEN0562_ADDR, (size_t)2);
    if (Wire.available() < 2) {
        logError(ERR_INVALID_DATA, "SEN0562");
        return SENSOR_LUX_ERROR_VALUE;
    }
    buf[0] = Wire.read();
    buf[1] = Wire.read();
    uint16_t data = (buf[0] << 8) | buf[1];
    return (uint16_t)(((float)data) / 1.2f);
}