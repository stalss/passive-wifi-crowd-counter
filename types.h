#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// ============================================================
//  Shared Data Structures
// ============================================================

// One slot in the fixed-size MAC hash table.
struct DeviceSlot {
    uint8_t       mac[6];        // raw 6-byte source MAC
    int8_t        rssi;          // most recent RSSI reading
    unsigned long lastSeenMs;    // millis() timestamp
    bool          occupied;      // true = slot in use
};

// Snapshot of current statistics, used by the reporter.
struct CrowdStats {
    uint16_t currentCount;       // unique MACs right now
    uint16_t peakCount;          // all-time high since boot
    uint16_t rollingAvg;         // smoothed count
    uint16_t slotsUsed;          // hash table slots occupied
    uint16_t slotsMax;           // hash table capacity
    uint8_t  currentChannel;     // WiFi channel being scanned
    int      rssiThreshold;      // active RSSI filter
    unsigned long uptimeMs;      // millis() since boot
};

#endif // TYPES_H
