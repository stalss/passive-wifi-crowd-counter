#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <stdint.h>

// ============================================================
//  Channel Hopping Manager
// ============================================================
//
//  Non-blocking channel hopper that cycles through WiFi
//  channels 1–13 at a configurable rate.
//
// ============================================================

// Initialise the channel manager and set the first channel.
void channelManagerInit();

// Call from loop().  Hops to the next channel when the
// dwell timer expires.
void channelManagerHop();

// Current channel number (1–13).
uint8_t channelManagerCurrent();

// Per-channel hit counters (from ISR).
// Index 0 is unused; channels 1–13 are valid.
uint16_t* channelManagerHits();

// Reset all hit counters.
void channelManagerResetHits();

#endif // CHANNEL_MANAGER_H
