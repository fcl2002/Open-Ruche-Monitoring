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
	result.humidity = (int8_t)(dht.readHumidity());
	result.temperature = (int16_t)(dht.readTemperature()*10);
	return result;
}

/* DS18B20 */
int16_t read_ds18b20_sonde(DeviceAddress sensorAddr) {
    sondes.requestTemperatures();
    if (sondes.isConnected(sensorAddr)) {
        return (int16_t)(sondes.getTempC(sensorAddr)*10);
    } else {
        Serial.print("Sensor not connected!");
        return -32768;
    }
}

/* SEN0592 */
uint16_t read_sen0562() {
    uint8_t buf[2] = {0};
    Wire.beginTransmission(SEN0562_ADDR);
    Wire.write(0x10);
    if (Wire.endTransmission() != 0) return 0;
    delay(20);
    Wire.requestFrom(SEN0562_ADDR, (uint8_t)2);
    for (uint8_t i = 0; i < 2; i++) {
        buf[i] = Wire.read();
    }
    uint16_t data = (buf[0] << 8) | buf[1];
    float lux = ((float)data) / 1.2;
    return (uint16_t)lux;
}