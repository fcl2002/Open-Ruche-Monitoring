/**
 *
 * Deep sleep management for Open Ruche Project
 * https://github.com/fcl2002/Open-Ruche-Monitoring
 *
 * MIT License
 * (c) 2026 Fernando Lasmar
 *
**/
#ifndef SLEEP_H
#define SLEEP_H

#include <stdint.h>

// Log the wakeup reason to Serial
void print_wakeup_reason();

// Release GPIO holds set on previous sleep cycle (call before Wire.begin)
void sleep_gpio_release();

// Enable voltage regulators (GPIO 12 & 13 HIGH) — call right after sleep_gpio_release()
void vreg_power_on();

// Enter deep sleep for the given number of seconds (does not return).
// Use DEEP_SLEEP_DURATION_S for normal cycles and DEEP_SLEEP_DORMANT_S
// for dormant (night/cold) mode.
void enter_deep_sleep(uint32_t duration_s);

#endif /* SLEEP_H */
