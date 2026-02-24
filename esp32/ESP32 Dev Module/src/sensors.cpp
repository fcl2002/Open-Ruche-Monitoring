#include <Arduino.h>
#include "sensors.h"

HX711 hx711;

unsigned long lastReadDht = 0;
DHT external_dht(EXTERNAL_DTH22_DATA_PIN, DHT22);
DHT internal_dht(INTERNAL_DTH22_DATA_PIN, DHT22);

int16_t read_hx711() {
	hx711.power_up();
	float units = hx711.get_units(10);
	Serial.print("Weight: ");
	Serial.print(units, 2);
	Serial.println(" kg");
	hx711.power_down();

	return (int16_t)(units*10);
}

DHT22Result read_dht22(DHT& dht, const char* sensorName){
    // int16_t hum = (int8_t)(dht.readHumidity());
    // int16_t temp = (int16_t)(dht.readTemperature()*10);
	// Serial.println(sensorName);
    // Serial.print("Humidity: ");
    // Serial.print(hum);
    // Serial.print(" %\t");
    // Serial.print("Temperature: ");
    // Serial.print(temp);
    // Serial.println(" *C");

	DHT22Result result;
	result.humidity = (int8_t)(dht.readHumidity());
	result.temperature = (int16_t)(dht.readTemperature()*10);
	return result;
}