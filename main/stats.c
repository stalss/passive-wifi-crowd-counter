#include "stats.h"
#include "config.h"

#include <string.h>

/* ================================================================
 *  Rolling Average & Peak Tracker
 * ================================================================ */

static uint16_t g_avgBuf[AVG_WINDOW];
static uint8_t  g_avgIdx    = 0;
static int      g_avgFilled = 0;
static uint16_t g_peak      = 0;

void statsInit(void) {
    memset(g_avgBuf, 0, sizeof(g_avgBuf));
    g_avgIdx    = 0;
    g_avgFilled = 0;
    g_peak      = 0;
}

void statsReset(void) {
    statsInit();
}

void statsPushSample(uint16_t value) {
    g_avgBuf[g_avgIdx] = value;
    g_avgIdx = (g_avgIdx + 1) % AVG_WINDOW;
    if (g_avgIdx == 0) g_avgFilled = 1;
}

uint16_t statsRollingAvg(void) {
    uint32_t sum = 0;
    uint8_t  n   = g_avgFilled ? AVG_WINDOW : g_avgIdx;
    if (n == 0) return 0;
    for (uint8_t i = 0; i < n; i++) sum += g_avgBuf[i];
    return (uint16_t)(sum / n);
}

uint16_t statsUpdatePeak(uint16_t current) {
    if (current > g_peak) g_peak = current;
    return g_peak;
}

uint16_t statsPeak(void) {
    return g_peak;
}
