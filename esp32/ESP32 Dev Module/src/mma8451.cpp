#include "mma8451.h"

Adafruit_MMA8451 mma = Adafruit_MMA8451();

static uint8_t MMA8451_ReadReg(uint8_t reg) {
    Wire.beginTransmission(MMA8451_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MMA8451_ADDR, (uint8_t)1);
    return Wire.read();
}

static void MMA8451_WriteReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MMA8451_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

static void MMA8451_Active() {
    MMA8451_WriteReg(MMA8451_REG_CTRL_REG1,
                     MMA8451_ReadReg(MMA8451_REG_CTRL_REG1) | 0x01);
}

void init_mma8451() {
    if (!mma.begin(MMA8451_ADDR)) {
        logError(ERR_DEVICE_NOT_FOUND, "MMA8451");
    } else {
        // Oversampling Mode: LOW POWER — MODS = 11
        MMA8451_Standby();

        // Clear the MODS bits in CTRL_REG2
        uint8_t reg2 = MMA8451_ReadReg(MMA8451_REG_CTRL_REG2);
        reg2 &= ~0x03; // ~MODS_MASK

        // Set the MODS bits to 11 for Low Power Mode
        reg2 |= 0x03;  // MODS_MASK
        MMA8451_WriteReg(MMA8451_REG_CTRL_REG2, reg2);

        MMA8451_Active();
        logInfo("MMA8451 initialized: low power, ODR=1.56Hz, range=2G", "SETUP");
    }
}

AccelResult read_mma8451() {
    mma.read();
    // Get sensor event with acceleration data
    sensors_event_t event;
    mma.getEvent(&event);
    
    AccelResult result;
    // Convert m/s^2 to int16_t (multiply by 10 for one decimal precision)
    result.x = (int16_t)(event.acceleration.x * 10);
    result.y = (int16_t)(event.acceleration.y * 10);
    result.z = (int16_t)(event.acceleration.z * 10);
    
    return result;
}

void MMA8451_Standby() {
    MMA8451_WriteReg(MMA8451_REG_CTRL_REG1,
                     MMA8451_ReadReg(MMA8451_REG_CTRL_REG1) & ~0x01);
}