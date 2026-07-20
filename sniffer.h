#ifndef SNIFFER_H
#define SNIFFER_H

#include <stdint.h>
#include "config.h"

// ============================================================
//  802.11 Packet Sniffer Callback
// ============================================================
//
//  Registers a promiscuous-mode callback with the ESP32
//  Wi-Fi driver.  The callback filters for Probe Requests
//  (subtype 0x04), extracts the source MAC and RSSI, and
//  feeds them into the hash table.
//
//  802.11 Management Frame Header Layout:
//
//    Offset  Field           Description
//    ------  --------------- ----------------------------------
//     0       Frame Control   Byte 0 bits[7:4] = subtype
//     2       Duration / ID
//     4       Address 1       Destination MAC
//    10       Address 2       Source MAC  ← we extract this
//    16       Address 3       BSSID
//
//  RSSI is NOT in the frame — it's in the driver metadata
//  struct (wifi_promiscuous_pkt_t.rx_ctrl.rssi).
//
// ============================================================

// Initialise the sniffer: set STA mode, enter promiscuous
// mode, and register the callback.
void snifferInit();

// Get/set the RSSI threshold at runtime.
int  snifferGetRssiThreshold();
void snifferSetRssiThreshold(int value);

// Pause / resume capturing (callback still fires but
// discards all packets).
void snifferPause();
void snifferResume();
bool snifferIsPaused();

#endif // SNIFFER_H
