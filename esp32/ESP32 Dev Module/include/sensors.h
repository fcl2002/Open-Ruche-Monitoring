/**
 *
 * IO library for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#ifndef SENSORS_h
#define SENSORS_h

#include "DHT.h"
#include <Wire.h>
#include "HX711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "errors.h"

// Error values for sensors — defined in config.h

extern HX711 hx711;

extern DHT external_dht;
extern DHT internal_dht;

extern OneWire oneWire;
extern DallasTemperature sondes;
extern DeviceAddress sonde1, sonde2;

struct DHT22Result {
	int8_t humidity;	  // 55.3% becomes 55
	int16_t temperature;  // 23.4°C becomes 234
};

// init sensors
void hx711_init();
void sondes_init();
void sensors_init();

// returns the weight of the hive from the HX711 module
uint16_t read_hx711();
		
// returns temperature and humidity reading from DHT22 sensor
DHT22Result read_dht22(DHT& dht, const char* sensorName);

// returns the temperature inside the hive reading from DS18B20 sondes
int16_t read_ds18b20_sonde(DeviceAddress sensor, const char* sensorName);

// returns the luminosity outside the hive reading from SEN0562 sensor
uint16_t read_sen0562();

// returns battery voltage as uint8_t (V * 10), e.g. 42 = 4.2 V
uint8_t read_battery_v();

#endif /* SENSORS_h */