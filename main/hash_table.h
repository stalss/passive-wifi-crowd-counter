#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include "config.h"
#include "types.h"

void     hashTableInit(void);
int      hashTableUpsert(const uint8_t *mac, int8_t rssi);
void     hashTableExpire(unsigned long now, unsigned long expiryMs);
uint16_t hashTableCount(void);
uint16_t hashTableCapacity(void);
DeviceSlot *hashTableSlots(void);
uint16_t hashTableSize(void);
void     hashTableReset(void);

#endif /* HASH_TABLE_H */
