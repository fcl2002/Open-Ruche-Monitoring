#include <Arduino.h>
#include "sensors.h"

HX711 hx711;

unsigned long lastRead = 0;
DHT external_dht(EXTERNAL_DTH22_DATA_PIN, DHT22);
DHT internal_dht(INTERNAL_DTH22_DATA_PIN, DHT22);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sondes(&oneWire);
DeviceAddress sonde1 = {0x28, 0x4C, 0xB5, 0x68, 0x10, 0x00, 0x00, 0x4D}; //fil orange
DeviceAddress sonde2 = {0x28, 0x33, 0xBA, 0x69, 0x10, 0x00, 0x00, 0x11};

Adafruit_MMA8451 mma = Adafruit_MMA8451();

/* HX711 */
int16_t read_hx711() {
	hx711.power_up();
	float units = hx711.get_units(10);
	Serial.print("Weight: ");
	Serial.print(units, 2);
	Serial.println(" kg");
	hx711.power_down();
	return (int16_t)(units*10);
}

/* DHT22 */
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

/* DS18B20 */
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

/* SEN0592 */
uint16_t read_sen0562() {
    uint8_t buf[2] = {0};
    Wire.beginTransmission(SEN0562_ADDR);
    Wire.write(0x10);
    if (Wire.endTransmission() != 0) {
        logError(ERR_I2C_COMMUNICATION_FAILED, "SEN0562");
        return SENSOR_LUX_ERROR_VALUE;
    }
    delay(20);
    size_t bytesToRequest = 2;
    Wire.requestFrom(SEN0562_ADDR, bytesToRequest);
    if (Wire.available() < 2) {
        logError(ERR_INVALID_DATA, "SEN0562");
        return SENSOR_LUX_ERROR_VALUE;
    }
    for (uint8_t i = 0; i < 2; i++) {
        buf[i] = Wire.read();
    }
    uint16_t data = (buf[0] << 8) | buf[1];
    float lux = ((float)data) / 1.2;
    return (uint16_t)lux;
}

/* MMA8451 Accelerometer */
AccelResult read_mma8451() {
    AccelResult result;
    
    mma.read();
    
    // Get sensor event with acceleration data
    sensors_event_t event;
    mma.getEvent(&event);
    
    // Convert m/s^2 to int16_t (multiply by 10 for one decimal precision)
    result.x = (int16_t)(event.acceleration.x * 10);
    result.y = (int16_t)(event.acceleration.y * 10);
    result.z = (int16_t)(event.acceleration.z * 10);
    
    return result;
}