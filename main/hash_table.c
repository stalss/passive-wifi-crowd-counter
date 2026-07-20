#include "hash_table.h"
#include <string.h>

/* ================================================================
 *  Fixed-Size Hash Table — Open Addressing, FNV-1a
 *
 *  ISR-safe: zero heap allocation, O(1) amortised upsert.
 * ================================================================ */

static DeviceSlot g_table[HASH_TABLE_SIZE];
static volatile uint16_t g_count = 0;

/* ── FNV-1a hash ────────────────────────────────────────────────── */

static inline uint32_t fnv1a(const uint8_t *mac) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < 6; i++) {
        h ^= mac[i];
        h *= 16777619u;
    }
    return h % HASH_TABLE_SIZE;
}

/* ── Byte-wise MAC comparison ───────────────────────────────────── */

static inline int macEqual(const uint8_t *a, const uint8_t *b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] &&
           a[3] == b[3] && a[4] == b[4] && a[5] == b[5];
}

/* ── Public API ─────────────────────────────────────────────────── */

void hashTableInit(void) {
    memset(g_table, 0, sizeof(g_table));
    g_count = 0;
}

void hashTableReset(void) {
    memset(g_table, 0, sizeof(g_table));
    g_count = 0;
}

int hashTableUpsert(const uint8_t *mac, int8_t rssi) {
    uint32_t idx = fnv1a(mac);
    unsigned long now = millis();

    for (uint32_t probe = 0; probe < HASH_TABLE_SIZE; probe++) {
        uint32_t pos = (idx + probe) % HASH_TABLE_SIZE;

        if (g_table[pos].occupied) {
            if (macEqual(g_table[pos].mac, mac)) {
                g_table[pos].rssi       = rssi;
                g_table[pos].lastSeenMs = now;
                return 0; /* refreshed */
            }
        } else {
            if (g_count < MAX_TRACKED_DEVICES) {
                memcpy(g_table[pos].mac, mac, 6);
                g_table[pos].rssi       = rssi;
                g_table[pos].lastSeenMs = now;
                g_table[pos].occupied   = 1;
                g_count++;
                return 1; /* new device */
            }
            return 0; /* table full */
        }
    }
    return 0;
}

void hashTableExpire(unsigned long now, unsigned long expiryMs) {
    for (uint32_t i = 0; i < HASH_TABLE_SIZE; i++) {
        if (g_table[i].occupied &&
            (now - g_table[i].lastSeenMs > expiryMs))
        {
            g_table[i].occupied = 0;
            g_count--;
        }
    }
}

uint16_t hashTableCount(void) {
    return g_count;
}

uint16_t hashTableCapacity(void) {
    return MAX_TRACKED_DEVICES;
}

DeviceSlot *hashTableSlots(void) {
    return g_table;
}

uint16_t hashTableSize(void) {
    return HASH_TABLE_SIZE;
}
