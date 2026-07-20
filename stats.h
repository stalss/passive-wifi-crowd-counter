#ifndef STATS_H
#define STATS_H

#include <stdint.h>

// ============================================================
//  Rolling Average & Peak Tracker
// ============================================================

// Initialise the statistics subsystem.
void statsInit();

// Push a new sample into the rolling average window.
void statsPushSample(uint16_t value);

// Get the current rolling average.
uint16_t statsRollingAvg();

// Peak tracker — call with the current count each cycle.
// Returns the updated peak.
uint16_t statsUpdatePeak(uint16_t current);

// Get the all-time peak.
uint16_t statsPeak();

// Reset all statistics.
void statsReset();

#endif // STATS_H
