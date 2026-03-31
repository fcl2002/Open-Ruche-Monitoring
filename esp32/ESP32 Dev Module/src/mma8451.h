/**
 * MMA8451 — Freefall/Motion detection driver
 * Standalone, no external libraries, raw I2C via Wire.
 *
 * INT strategy: push-pull, active-LOW.
 *   Idle  → MMA8451 drives INT1 HIGH (no pull-up needed)
 *   Event → MMA8451 drives INT1 LOW
 *   ESP32 EXT0 wakes on LOW level.
 *
 *   Why active-LOW and not active-HIGH:
 *   The MMA8451 power-on default is IPOL=0 (active-LOW), push-pull.
 *   If the sensor resets during ESP32 deep sleep, INT1 returns to
 *   HIGH (inactive for active-LOW) — which does NOT trigger EXT0
 *   wake-on-LOW.  Using active-HIGH would cause the default state
 *   (HIGH) to immediately wake the ESP32 in a boot loop.
 */
#ifndef MMA8451_H
#define MMA8451_H

#include <stdint.h>
#include <stdbool.h>
#include <Arduino.h>

/* ── I2C address ───────────────────────────────────────────── */
#define MMA8451_ADDR        0x1D   /* SA0 pin = VDD */

/* ── ESP32 pin wired to MMA8451 INT1 ──────────────────────── */
#define MMA8451_INT_PIN     35

/* ── Register map ─────────────────────────────────────────── */
#define REG_WHO_AM_I        0x0D
#define REG_INT_SOURCE      0x0C   /* Read-only, clears with source reads */
#define REG_FF_MT_CFG       0x15
#define REG_FF_MT_SRC       0x16   /* Reading this clears the latch */
#define REG_FF_MT_THS       0x17
#define REG_FF_MT_COUNT     0x18
#define REG_CTRL_REG1       0x2A
#define REG_CTRL_REG2       0x2B
#define REG_CTRL_REG3       0x2C
#define REG_CTRL_REG4       0x2D
#define REG_CTRL_REG5       0x2E

/* ── Motion tuning ────────────────────────────────────────── */
/*
 * FF_MT_THS: 1 LSB = 0.063 g  (range ±2g, 7-bit field)
 *   0x04 =  4 × 0.063 = 0.25 g  ← TEST: any noticeable tap/tilt
 *   0x08 =  8 × 0.063 = 0.50 g  (moderate shake)
 *   0x10 = 16 × 0.063 = 1.01 g  (large displacement — production)
 *
 * Z-axis DISABLED: gravity (~1 g) would always exceed low thresholds.
 * X and Y detect horizontal shake and tilt from vertical.
 * To trigger: tilt the board sideways OR shake it horizontally.
 *
 * FF_MT_COUNT: debounce at 50 Hz → 1 count = 20 ms.
 *   0x02 =  2 counts =  40 ms  ← TEST: triggers fast
 *   0x0A = 10 counts = 200 ms  (sustained motion — production)
 */
#define MOTION_THS          0x04   /* 0.25 g — easy to trigger for testing */
#define MOTION_COUNT        0x02   /* 40 ms debounce at 50 Hz */

/* ── INT_SOURCE bits ──────────────────────────────────────── */
#define INT_SRC_FF_MT       (1 << 2)

/* ── FF_MT_SRC bits ───────────────────────────────────────── */
#define FF_MT_EA            (1 << 7)   /* Event Active */
#define FF_MT_ZHE           (1 << 5)   /* Z exceeded threshold */
#define FF_MT_YHE           (1 << 3)   /* Y exceeded threshold */
#define FF_MT_XHE           (1 << 1)   /* X exceeded threshold */

/* ── Low-level I2C ────────────────────────────────────────── */
bool     mma8451_read_reg(uint8_t reg, uint8_t *val);
bool     mma8451_write_reg(uint8_t reg, uint8_t val);

/* ── Sensor lifecycle ─────────────────────────────────────── */
bool     mma8451_init(void);             /* Verify WHO_AM_I */
bool     mma8451_configure_motion(void); /* Full register setup */

/* ── Interrupt handling ───────────────────────────────────── */
uint8_t  mma8451_read_int_source(void);  /* INT_SOURCE (0x0C) */
uint8_t  mma8451_read_motion_src(void);  /* FF_MT_SRC — also clears latch */

/* ── System wake ──────────────────────────────────────────── */
void     mma8451_arm_wakeup(void);       /* Configure EXT0 on INT_PIN */

#endif /* MMA8451_H */
