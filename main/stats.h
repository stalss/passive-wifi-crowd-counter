#ifndef STATS_H
#define STATS_H

#include <stdint.h>

void     statsInit(void);
void     statsPushSample(uint16_t value);
uint16_t statsRollingAvg(void);
uint16_t statsUpdatePeak(uint16_t current);
uint16_t statsPeak(void);
void     statsReset(void);

#endif /* STATS_H */
