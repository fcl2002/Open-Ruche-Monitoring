#ifndef BOARDS_H
#define BOARDS_H

#include <stdint.h>

// Initialise UART1 to receive hive state from the AI board.
// Call once in setup() before read_audio().
void boards_init(void);

// Read the hive state sent by the microphone AI board over UART.
// Blocks for up to AI_READ_TIMEOUT_MS waiting for a byte.
// Returns: 1 = normal, 2 = swarming, 3 = missing queen, 0 = timeout/invalid.
uint8_t read_audio(void);

// Read the value sent by the camera AI board over SoftwareSerial.
// Blocks for up to CAM_READ_TIMEOUT_MS waiting for a byte.
// Returns the raw uint8 value, or 0 on timeout.
uint8_t read_camera(void);

#endif /* BOARDS_H */
