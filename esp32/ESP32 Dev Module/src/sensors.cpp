#include <Arduino.h>
#include "sensors.h"
#include "mma8451.h"

HX711 hx711;

DHT external_dht(EXTERNAL_DTH22_DATA_PIN, DHT22);
DHT internal_dht(INTERNAL_DTH22_DATA_PIN, DHT22);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sondes(&oneWire);
DeviceAddress sonde1 = {0x28, 0x4C, 0xB5, 0x68, 0x10, 0x00, 0x00, 0x4D}; //fil orange
DeviceAddress sonde2 = {0x28, 0x33, 0xBA, 0x69, 0x10, 0x00, 0x00, 0x11};

void init_sensors() {
    init_hx711();
	external_dht.begin();
	internal_dht.begin();
    logInfo("DHT22 sensors initialized", "SETUP");
    init_ds18b20();
    init_mma8451();
}

void init_hx711(void) {
    hx711.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
	vTaskDelay(200);
	hx711.set_scale(HX711_SCALE);
	hx711.set_offset(HX711_OFFSET);
	vTaskDelay(200);
    logInfo("HX711 initialized: scale=30148, offset=134750", "SETUP");
}

void init_ds18b20(void) {
    sondes.begin();
    int deviceCount = sondes.getDeviceCount();
    
    if (deviceCount > 0)
        sondes.getAddress(sonde1, 0);
    else
        logError(ERR_DEVICE_NOT_FOUND, "DS18B20 Sonde 1");
    
    if (deviceCount > 1)
        sondes.getAddress(sonde2, 1);
    else if (deviceCount == 1)
        logError(ERR_DEVICE_NOT_FOUND, "DS18B20 Sonde 2");

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