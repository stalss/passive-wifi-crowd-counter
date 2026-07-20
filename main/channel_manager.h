#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <stdint.h>

void     channelManagerInit(void);
void     channelManagerHop(void);
uint8_t  channelManagerCurrent(void);
uint16_t *channelManagerHits(void);
void     channelManagerResetHits(void);

#endif /* CHANNEL_MANAGER_H */
