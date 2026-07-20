#ifndef REPORTER_H
#define REPORTER_H

#include "types.h"

void reporterInit(void);
void reporterBanner(void);
int  reporterPrint(const CrowdStats *stats);
void reporterSetCsvMode(int enabled);
int  reporterIsCsvMode(void);
void reporterCsvHeader(void);

#endif /* REPORTER_H */
