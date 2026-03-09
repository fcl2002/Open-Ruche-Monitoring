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

// Log the wakeup reason to Serial
void print_wakeup_reason();

// Release GPIO holds set on previous sleep cycle (call before Wire.begin)
void sleep_gpio_release();

// Enter deep sleep for the configured duration (does not return)
void enter_deep_sleep();

#endif /* SLEEP_H */
