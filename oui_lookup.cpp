#include "oui_lookup.h"

// ============================================================
//  OUI Vendor Lookup — Implementation
// ============================================================
//
//  This is a small, curated table.  For a full database see
//  https://standards-oui.ieee.org/ (28,000+ entries).
//  We only include the most common prefixes for the device
//  categories we care about.
//
//  To add a new entry: append to g_ouiTable and increment
//  OUI_TABLE_SIZE in config.h.
//
// ============================================================

static const OuiEntry g_ouiTable[OUI_TABLE_SIZE] = {

    // ── Phones / Tablets ──────────────────────────────────

    // Apple (iPhone, iPad, MacBook Wi-Fi)
    { {0x00, 0x1A, 0x2B}, "Apple",         VENDOR_PHONE  },
    { {0x3C, 0x22, 0xFB}, "Apple",         VENDOR_PHONE  },

    // Samsung Galaxy
    { {0x00, 0x1E, 0x58}, "Samsung",       VENDOR_PHONE  },
    { {0x30, 0x96, 0xFB}, "Samsung",       VENDOR_PHONE  },

    // Google Pixel
    { {0x3C, 0x28, 0x6D}, "Google",        VENDOR_PHONE  },

    // ── Laptops / PCs ────────────────────────────────────

    // Intel Wi-Fi adapters (very common in laptops)
    { {0x00, 0x13, 0x02}, "Intel",         VENDOR_LAPTOP },
    { {0x3C, 0x97, 0x0E}, "Intel",         VENDOR_LAPTOP },

    // Realtek (budget laptops, USB adapters)
    { {0x00, 0xE0, 0x4C}, "Realtek",       VENDOR_LAPTOP },

    // Broadcom (MacBook, many Windows laptops)
    { {0x00, 0x10, 0x18}, "Broadcom",      VENDOR_LAPTOP },

    // ── IoT / Embedded ───────────────────────────────────

    // Espressif (ESP32, ESP8266 — other IoT devices!)
    { {0x30, 0xAE, 0xA4}, "Espressif",     VENDOR_IOT    },

    // Tuya (smart plugs, lights, sensors)
    { {0x10, 0xD5, 0x42}, "Tuya",          VENDOR_IOT    },

    // Xiaomi (Mi Home devices)
    { {0x78, 0x11, 0xDC}, "Xiaomi IoT",    VENDOR_IOT    },
};

// ── Lookup function ────────────────────────────────────────

const OuiEntry* ouiLookup(const uint8_t *mac) {
    for (uint8_t i = 0; i < OUI_TABLE_SIZE; i++) {
        if (mac[0] == g_ouiTable[i].prefix[0] &&
            mac[1] == g_ouiTable[i].prefix[1] &&
            mac[2] == g_ouiTable[i].prefix[2])
        {
            return &g_ouiTable[i];
        }
    }
    return NULL;
}

const char* ouiTypeName(VendorType type) {
    switch (type) {
        case VENDOR_PHONE:  return "Phone";
        case VENDOR_LAPTOP: return "Laptop";
        case VENDOR_IOT:    return "IoT";
        default:            return "Unknown";
    }
}
