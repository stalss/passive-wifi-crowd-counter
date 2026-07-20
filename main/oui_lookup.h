#ifndef OUI_LOOKUP_H
#define OUI_LOOKUP_H

#include <stdint.h>

/* ================================================================
 *  OUI Vendor Prefix Lookup
 * ================================================================ */

typedef enum {
    VENDOR_UNKNOWN = 0,
    VENDOR_PHONE,
    VENDOR_LAPTOP,
    VENDOR_IOT
} VendorType;

typedef struct {
    uint8_t     prefix[3];
    const char *name;
    VendorType  type;
} OuiEntry;

const OuiEntry *ouiLookup(const uint8_t *mac);
const char     *ouiTypeName(VendorType type);

#endif /* OUI_LOOKUP_H */
