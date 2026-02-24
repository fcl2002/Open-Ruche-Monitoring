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
#include "HX711.h"

#define HX711_DOUT_PIN 32 // DT pin
#define HX711_SCK_PIN 33  // SCK pin

#define EXTERNAL_DTH22_DATA_PIN 27 // external DHT pin
#define INTERNAL_DTH22_DATA_PIN 26 // internal DHT pin

#define TIME_SETUP 5000
#define TIME_DHT22 10000

extern HX711 hx711;

extern DHT external_dht;
extern DHT internal_dht;
extern unsigned long lastReadDht;

struct DHT22Result {
	int8_t humidity;	  // 55.3% becomes 55
	int16_t temperature;  // 23.4% becomes 234
};

// returns the weight of the hive from the HX711 module
int16_t read_hx711();
		
// returns temperature and humidity reading from DHT22 sensor
DHT22Result read_dht22(DHT& dht, const char* sensorName);

#endif /* SENSORS_h */