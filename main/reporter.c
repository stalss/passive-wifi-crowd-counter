#include "reporter.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

/* ================================================================
 *  Serial Reporter — Human-readable + CSV modes
 * ================================================================ */

static int          g_csvMode = 0;
static unsigned long g_lastReportMs = 0;

void reporterInit(void) {
    g_lastReportMs = millis();
    g_csvMode = 0;
}

void reporterCsvHeader(void) {
    printf("timestamp_ms,count,peak,avg,rssi_thresh,channel,slots_used\r\n");
}

static void printHuman(const CrowdStats *s) {
    char uptime[12];
    fmtUptime(s->uptimeMs, uptime, sizeof(uptime));

    printf("--------------------------------------------------\r\n");
    printf("[%s]  Crowd: %u  (avg: %u, peak: %u)  RSSI >= %d dBm  ch: %u\r\n",
           uptime, s->currentCount, s->rollingAvg, s->peakCount,
           s->rssiThreshold, s->currentChannel);
    printf("  Slots: %u/%u\r\n", s->slotsUsed, s->slotsMax);
}

static void printCsv(const CrowdStats *s) {
    printf("%lu,%u,%u,%u,%d,%u,%u\r\n",
           s->uptimeMs, s->currentCount, s->peakCount,
           s->rollingAvg, s->rssiThreshold,
           s->currentChannel, s->slotsUsed);
}

int reporterPrint(const CrowdStats *stats) {
    if (millis() - g_lastReportMs < REPORT_INTERVAL_MS) return 0;
    g_lastReportMs = millis();

    if (g_csvMode) printCsv(stats);
    else           printHuman(stats);

    return 1;
}

void reporterSetCsvMode(int enabled) {
    g_csvMode = enabled;
    if (enabled) reporterCsvHeader();
}

int reporterIsCsvMode(void) {
    return g_csvMode;
}
