#ifndef SNIFFER_H
#define SNIFFER_H

#include <stdint.h>

void snifferInit(void);
int  snifferGetRssiThreshold(void);
void snifferSetRssiThreshold(int value);
void snifferPause(void);
void snifferResume(void);
int  snifferIsPaused(void);

#endif /* SNIFFER_H */
