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
	// Serial.println(sensorName);
    // Serial.print("Humidity: ");
    // Serial.print((int8_t)(dht.readHumidity()));
    // Serial.print(" %\t");
    // Serial.print("Temperature: ");
    // Serial.print((int16_t)(dht.readTemperature()*10));
    // Serial.println(" *C");

	DHT22Result result;
	result.humidity = (int8_t)(dht.readHumidity());
	result.temperature = (int16_t)(dht.readTemperature()*10);
	return result;
}

/* Sondes DS18B20 */
void printAddress(DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i++) {
        if (deviceAddress[i] < 16) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

int16_t read_ds18b20_sonde(DeviceAddress sensorAddr) {
    sondes.requestTemperatures();
    if (sondes.isConnected(sensorAddr)) {
        // uint16_t temp = (int16_t)(sensors.getTempC(sensorAddr)*10);
        // Serial.print("Sensor [");
        // printAddress(sensorAddr);
        // Serial.print("]: ");
        // Serial.print(temp);
        // Serial.println(" °C");
        return (int16_t)(sondes.getTempC(sensorAddr)*10);
    } else {
        Serial.print("Sensor [");
        printAddress(sensorAddr);
        Serial.println("] not connected!");
        return -32768;
    }
}