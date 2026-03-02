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
#include "Wire.h"
#include "HX711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include "errors.h"

// Error values for sensors
#define SENSOR_ERROR_VALUE -32768
#define SENSOR_LUX_ERROR_VALUE 0
#define ACCEL_ERROR_VALUE -32768

#define HX711_DOUT_PIN 32 // DT pin
#define HX711_SCK_PIN 33  // SCK pin

#define EXTERNAL_DTH22_DATA_PIN 27 // external DHT22 pin
#define INTERNAL_DTH22_DATA_PIN 26 // internal DHT22 pin

#define ONE_WIRE_BUS 4 // sondes DS18B20 pin

#define SEN0562_ADDR 0x23 // SEN0562 I2C address
#define MMA8451_ADDR 0x1C // MMA8451 I2C address

#define TIME_SETUP 5000
#define TIME_TO_READ 5000

extern HX711 hx711;

extern DHT external_dht;
extern DHT internal_dht;
extern unsigned long lastRead;

extern OneWire oneWire;
extern DallasTemperature sondes;
extern DeviceAddress sonde1, sonde2;

extern Adafruit_MMA8451 mma;

struct DHT22Result {
	int8_t humidity;	  // 55.3% becomes 55
	int16_t temperature;  // 23.4% becomes 234
};

struct AccelResult {
	int16_t x;  // X-axis acceleration (m/s^2 * 10)
	int16_t y;  // Y-axis acceleration (m/s^2 * 10)
	int16_t z;  // Z-axis acceleration (m/s^2 * 10)
};

// returns the weight of the hive from the HX711 module
uint16_t read_hx711();
		
// returns temperature and humidity reading from DHT22 sensor
DHT22Result read_dht22(DHT& dht, const char* sensorName);

// returns the temperature inside the hive reading from DS18B20 sondes
int16_t read_ds18b20_sonde(DeviceAddress sensor, const char* sensorName);

// returns the luminosity outside the hive reading from SEN0562 sensor
uint16_t read_sen0562();

// returns the acceleration data from MMA8451 accelerometer
AccelResult read_mma8451();

#endif /* SENSORS_h */