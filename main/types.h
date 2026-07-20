#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* ================================================================
 *  Shared Data Structures
 * ================================================================ */

typedef struct {
    uint8_t       mac[6];
    int8_t        rssi;
    unsigned long lastSeenMs;
    int           occupied;
} DeviceSlot;

typedef struct {
    uint16_t currentCount;
    uint16_t peakCount;
    uint16_t rollingAvg;
    uint16_t slotsUsed;
    uint16_t slotsMax;
    uint8_t  currentChannel;
    int      rssiThreshold;
    unsigned long uptimeMs;
} CrowdStats;

#endif /* TYPES_H */
