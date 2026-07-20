#include "oui_lookup.h"
#include "config.h"

/* ================================================================
 *  OUI Vendor Lookup — Small curated table
 *
 *  To add entries: append to g_ouiTable and increment
 *  OUI_TABLE_SIZE in config.h.
 * ================================================================ */

static const OuiEntry g_ouiTable[OUI_TABLE_SIZE] = {
    /* Phones / Tablets */
    { {0x00, 0x1A, 0x2B}, "Apple",     VENDOR_PHONE  },
    { {0x3C, 0x22, 0xFB}, "Apple",     VENDOR_PHONE  },
    { {0x00, 0x1E, 0x58}, "Samsung",   VENDOR_PHONE  },
    { {0x30, 0x96, 0xFB}, "Samsung",   VENDOR_PHONE  },
    { {0x3C, 0x28, 0x6D}, "Google",    VENDOR_PHONE  },

    /* Laptops / PCs */
    { {0x00, 0x13, 0x02}, "Intel",     VENDOR_LAPTOP },
    { {0x3C, 0x97, 0x0E}, "Intel",     VENDOR_LAPTOP },
    { {0x00, 0xE0, 0x4C}, "Realtek",   VENDOR_LAPTOP },
    { {0x00, 0x10, 0x18}, "Broadcom",  VENDOR_LAPTOP },

    /* IoT / Embedded */
    { {0x30, 0xAE, 0xA4}, "Espressif", VENDOR_IOT    },
    { {0x10, 0xD5, 0x42}, "Tuya",      VENDOR_IOT    },
    { {0x78, 0x11, 0xDC}, "Xiaomi",    VENDOR_IOT    },
};

const OuiEntry *ouiLookup(const uint8_t *mac) {
    for (int i = 0; i < OUI_TABLE_SIZE; i++) {
        if (mac[0] == g_ouiTable[i].prefix[0] &&
            mac[1] == g_ouiTable[i].prefix[1] &&
            mac[2] == g_ouiTable[i].prefix[2])
        {
            return &g_ouiTable[i];
        }
    }
    return (void *)0;
}

const char *ouiTypeName(VendorType type) {
    switch (type) {
        case VENDOR_PHONE:  return "Phone";
        case VENDOR_LAPTOP: return "Laptop";
        case VENDOR_IOT:    return "IoT";
        default:            return "Unknown";
    }
}
