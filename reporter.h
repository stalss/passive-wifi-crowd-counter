#ifndef REPORTER_H
#define REPORTER_H

#include "types.h"

// ============================================================
//  Serial Reporter
// ============================================================
//
//  Formats and prints crowd-count data to the Serial monitor.
//  Supports two output modes:
//
//    1. Human-readable (default)
//    2. CSV (for data logging — pipe serial to a .csv file)
//
// ============================================================

// Initialise the reporter.
void reporterInit();

// Print the banner once at boot.
void reporterBanner();

// Print the periodic report.  Call from loop().
// Returns true if a report was actually printed.
bool reporterPrint(const CrowdStats &stats);

// Toggle between human-readable and CSV output.
void reporterSetCsvMode(bool enabled);
bool reporterIsCsvMode();

// Print a single CSV header line (column names).
void reporterCsvHeader();

#endif // REPORTER_H
