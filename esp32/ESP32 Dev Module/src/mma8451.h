/**
 *
 * IO library for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#ifndef MMA8451_H
#define MMA8451_H

#include <Arduino.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include "config.h"
#include "errors.h"
#include "logger.h"

struct AccelResult {
	int16_t x;  // X-axis acceleration (m/s^2 * 10)
	int16_t y;  // Y-axis acceleration (m/s^2 * 10)
	int16_t z;  // Z-axis acceleration (m/s^2 * 10)
};

extern Adafruit_MMA8451 mma;

void init_mma8451();
void MMA8451_Standby();

// returns the acceleration data from MMA8451 accelerometer
AccelResult read_mma8451();

#endif