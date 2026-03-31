/**
 * MMA8451 — Freefall/Motion detection driver implementation
 *
 * Detection mechanism: FF_MT (not Transient).
 *   FF_MT has no high-pass filter — it sees raw acceleration.
 *   Motion mode (OAE=1) fires when |accel| > threshold on any
 *   enabled axis.  Z-axis is excluded because gravity (~1 g)
 *   would permanently sit at or above a useful threshold.
 *
 * INT electrical: push-pull, active-HIGH.
 *   No external pull-up resistor required.
 *   Idle → INT1 LOW.  Event → INT1 HIGH.
 *   ESP32 EXT0 configured to wake on HIGH.
 */

#include "mma8451.h"
#include <Wire.h>
#include <esp_sleep.h>

/* ═══════════════════════════════════════════════════════════════
 * Low-level I2C
 * ═══════════════════════════════════════════════════════════════ */

bool mma8451_read_reg(uint8_t reg, uint8_t *val) {
    Wire.beginTransmission(MMA8451_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;   /* keep bus */

    Wire.requestFrom((uint8_t)MMA8451_ADDR, (uint8_t)1);
    if (!Wire.available()) return false;

    *val = Wire.read();
    return true;
}

bool mma8451_write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MMA8451_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

/* ═══════════════════════════════════════════════════════════════
 * Standby / Active helpers
 * The sensor must be in standby before any configuration register
 * can be written (CTRL_REG1..5, FF_MT_*).
 * ═══════════════════════════════════════════════════════════════ */

static bool standby(void) {
    uint8_t r;
    if (!mma8451_read_reg(REG_CTRL_REG1, &r)) return false;
    return mma8451_write_reg(REG_CTRL_REG1, r & ~0x01);  /* ACTIVE=0 */
}

static bool activate(void) {
    uint8_t r;
    if (!mma8451_read_reg(REG_CTRL_REG1, &r)) return false;
    return mma8451_write_reg(REG_CTRL_REG1, r | 0x01);   /* ACTIVE=1 */
}

/* ═══════════════════════════════════════════════════════════════
 * mma8451_init
 * Verify the sensor is present and responding on the I2C bus.
 * Call after Wire.begin().
 * ═══════════════════════════════════════════════════════════════ */

bool mma8451_init(void) {
    uint8_t who = 0;
    if (!mma8451_read_reg(REG_WHO_AM_I, &who)) {
        Serial.println("[MMA8451] I2C read failed");
        return false;
    }
    if (who != 0x1A) {
        Serial.printf("[MMA8451] Wrong WHO_AM_I: 0x%02X (expected 0x1A)\n", who);
        return false;
    }
    Serial.printf("[MMA8451] Found, WHO_AM_I=0x%02X\n", who);
    return true;
}

/* ═══════════════════════════════════════════════════════════════
 * mma8451_configure_motion
 *
 * Register layout used:
 *
 *  CTRL_REG1  (0x2A): ODR + standby/active
 *    DR[2:0] at bits [5:3].  DR=100 → 50 Hz.
 *    Written as 0x20 (standby, 50 Hz), then set ACTIVE bit last.
 *
 *  CTRL_REG2  (0x2B): oversampling mode
 *    MODS[1:0] = 11 → low-power mode.
 *
 *  FF_MT_CFG  (0x15): motion source config
 *    Bit 7: OAE  = 1 → motion detection (not freefall)
 *    Bit 6: ELE  = 1 → latch event until FF_MT_SRC is read
 *    Bit 5: ZEFE = 0 → Z disabled (gravity would falsely trigger)
 *    Bit 4: YEFE = 1 → Y-axis enabled
 *    Bit 3: XEFE = 1 → X-axis enabled
 *    Value: 0b11011000 = 0xD8
 *
 *  FF_MT_THS  (0x17): threshold
 *    Bit 7: DBCNTM = 0 → counter decrements on non-event
 *    Bits[6:0]: MOTION_THS (see header)
 *
 *  FF_MT_COUNT (0x18): debounce persistence counter
 *    MOTION_COUNT samples must exceed THS before event fires.
 *
 *  CTRL_REG3  (0x2C): interrupt pin electrical
 *    Bit 1: IPOL  = 1 → active-HIGH
 *    Bit 0: PP_OD = 0 → push-pull (MMA8451 drives the line both ways)
 *    Value: 0x02
 *
 *  CTRL_REG4  (0x2D): interrupt enable
 *    Bit 2: INT_EN_FF_MT = 1
 *    Value: 0x04
 *
 *  CTRL_REG5  (0x2E): interrupt routing
 *    Bit 2: INT_CFG_FF_MT = 1 → route FF_MT to INT1 (0 = INT2)
 *    Value: 0x04
 * ═══════════════════════════════════════════════════════════════ */

bool mma8451_configure_motion(void) {
    /* 1. Enter standby — required before writing config registers */
    if (!standby()) { Serial.println("[MMA8451] Standby failed"); return false; }
    delay(5);

    /* 2. ODR = 50 Hz, standby */
    /* CTRL_REG1: ASLP_RATE=00, DR=100 (50Hz), LNOISE=0, F_READ=0, ACTIVE=0 */
    if (!mma8451_write_reg(REG_CTRL_REG1, 0x20)) return false;

    /* 3. Low-power oversampling (reduces current between samples) */
    /* CTRL_REG2: SMODS=00, SLPE=0, MODS=11 */
    if (!mma8451_write_reg(REG_CTRL_REG2, 0x03)) return false;

    /* 4. Motion detection: X + Y only, event latch enabled */
    /* FF_MT_CFG: OAE=1, ELE=1, ZEFE=0, YEFE=1, XEFE=1 → 0xD8 */
    if (!mma8451_write_reg(REG_FF_MT_CFG, 0xD8)) return false;

    /* 5. Threshold: DBCNTM=0, THS=MOTION_THS */
    if (!mma8451_write_reg(REG_FF_MT_THS, MOTION_THS & 0x7F)) return false;

    /* 6. Debounce counter */
    if (!mma8451_write_reg(REG_FF_MT_COUNT, MOTION_COUNT)) return false;

    /* 7. INT pin: push-pull (PP_OD=0), active-LOW (IPOL=0) → 0x00
     *    Idle = HIGH, Event = LOW. Matches MMA8451 power-on default,
     *    so a sensor reset during sleep does not cause a false wakeup. */
    if (!mma8451_write_reg(REG_CTRL_REG3, 0x00)) return false;

    /* 8. Enable only the FF_MT interrupt */
    if (!mma8451_write_reg(REG_CTRL_REG4, 0x04)) return false;

    /* 9. Route FF_MT → INT1 */
    if (!mma8451_write_reg(REG_CTRL_REG5, 0x04)) return false;

    /* 10. Clear any pending latch so INT1 idles LOW before we sleep */
    uint8_t dummy;
    mma8451_read_reg(REG_FF_MT_SRC, &dummy);

    /* 11. Return to active */
    if (!activate()) { Serial.println("[MMA8451] Activate failed"); return false; }
    delay(5);

    /* Verify INT1 is HIGH (idle, active-LOW = no event) after configuration */
    int pin = digitalRead(MMA8451_INT_PIN);
    Serial.printf("[MMA8451] Config OK. INT1=GPIO%d=%s (expect HIGH)\n",
                  MMA8451_INT_PIN, pin ? "HIGH" : "LOW");

    return true;
}

/* ═══════════════════════════════════════════════════════════════
 * mma8451_read_int_source
 * Returns INT_SOURCE (0x0C) — bitmask of which interrupt fired.
 * Reading this register does NOT clear the source; reading the
 * individual source register (e.g. FF_MT_SRC) does.
 * ═══════════════════════════════════════════════════════════════ */

uint8_t mma8451_read_int_source(void) {
    uint8_t src = 0;
    mma8451_read_reg(REG_INT_SOURCE, &src);
    return src;
}

/* ═══════════════════════════════════════════════════════════════
 * mma8451_read_motion_src
 * Returns FF_MT_SRC (0x16).
 * SIDE EFFECT: reading this register releases the event latch,
 * which drives INT1 back LOW.  Must be called after every wake.
 * ═══════════════════════════════════════════════════════════════ */

uint8_t mma8451_read_motion_src(void) {
    uint8_t src = 0;
    mma8451_read_reg(REG_FF_MT_SRC, &src);
    return src;
}

/* ═══════════════════════════════════════════════════════════════
 * mma8451_arm_wakeup
 * Configure the ESP32 EXT0 source: wake when INT_PIN goes LOW.
 * INT1 is push-pull active-LOW: idle=HIGH, event=LOW.
 * ═══════════════════════════════════════════════════════════════ */

void mma8451_arm_wakeup(void) {
    /* Verify INT1 is HIGH (idle) before arming — if LOW, there is a
     * pending event that would cause an immediate wakeup after sleep. */
    int pin = digitalRead(MMA8451_INT_PIN);
    if (pin == LOW) {
        /* Clear the latch so INT1 returns HIGH before sleeping */
        uint8_t dummy;
        mma8451_read_reg(REG_FF_MT_SRC, &dummy);
        Serial.printf("[MMA8451] Cleared pending event before sleep (SRC=0x%02X)\n", dummy);
    }

    /* level=0: wake when GPIO is LOW (active-LOW interrupt from MMA8451) */
    esp_err_t err = esp_sleep_enable_ext0_wakeup((gpio_num_t)MMA8451_INT_PIN, 0);
    if (err != ESP_OK) {
        Serial.printf("[MMA8451] EXT0 config error: %s\n", esp_err_to_name(err));
    } else {
        Serial.printf("[MMA8451] EXT0 armed: GPIO%d LOW = motion\n", MMA8451_INT_PIN);
    }
}
