#ifndef OUI_LOOKUP_H
#define OUI_LOOKUP_H

#include <stdint.h>

// ============================================================
//  OUI (Organisationally Unique Identifier) Vendor Lookup
// ============================================================
//
//  Matches the first 3 bytes of a MAC address against a
//  small built-in table of known vendor prefixes to
//  categorise devices into types:
//
//    VENDOR_PHONE    — Apple, Samsung, Google, etc.
//    VENDOR_LAPTOP   — Intel, Realtek, Broadcom, etc.
//    VENDOR_IOT      — Espressif, Tuya, Xiaomi, etc.
//    VENDOR_UNKNOWN  — not in the table
//
// ============================================================

enum VendorType : uint8_t {
    VENDOR_UNKNOWN = 0,
    VENDOR_PHONE,
    VENDOR_LAPTOP,
    VENDOR_IOT
};

struct OuiEntry {
    uint8_t     prefix[3];       // first 3 bytes of MAC
    const char* name;            // human-readable vendor name
    VendorType  type;            // device category
};

// Look up a MAC address.  Returns a pointer to the matching
// OuiEntry, or NULL if no match.
const OuiEntry* ouiLookup(const uint8_t *mac);

// Get a human-readable string for a VendorType.
const char* ouiTypeName(VendorType type);

#endif // OUI_LOOKUP_H
