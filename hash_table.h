#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include "config.h"
#include "types.h"

// ============================================================
//  Fixed-Size Hash Table for MAC Deduplication
// ============================================================
//
//  Open-addressing hash table with FNV-1a hashing.
//  Designed for ISR-safe use: zero heap allocation,
//  O(1) amortised upsert, flat memory layout.
//
// ============================================================

// Initialise all slots to empty.
void hashTableInit();

// Insert a new MAC or refresh an existing entry.
// Returns true if a NEW device was added (not a refresh).
bool hashTableUpsert(const uint8_t *mac, int8_t rssi);

// Evict entries older than `expiryMs` milliseconds.
void hashTableExpire(unsigned long now, unsigned long expiryMs);

// Number of occupied slots.
uint16_t hashTableCount();

// Maximum capacity.
uint16_t hashTableCapacity();

// Direct access to the slot array (for dump / vendor scans).
DeviceSlot* hashTableSlots();

// Reset all slots to empty and zero the count.
void hashTableReset();

#endif // HASH_TABLE_H
