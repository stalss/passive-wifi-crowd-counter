# Passive WiFi Crowd Counter — ESP32

A **zero-cost, privacy-respecting** people counter that estimates room occupancy by passively sniffing Wi-Fi probe requests. No cameras, no beacons, no cloud — just a $3 ESP32 board and radio physics.

> **v3.0** — Modular architecture, serial CLI, OUI vendor lookup, CSV logging, rolling averages, and more.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [How It Works](#how-it-works)
3. [Hardware](#hardware)
4. [Software Setup](#software-setup)
5. [Project Structure](#project-structure)
6. [Serial CLI Reference](#serial-cli-reference)
7. [Configuration Reference](#configuration-reference)
8. [Serial Output Formats](#serial-output-formats)
9. [OUI Vendor Lookup](#oui-vendor-lookup)
10. [CSV Data Logging](#csv-data-logging)
11. [802.11 Header Parsing](#80211-header-parsing)
12. [Architecture & Data Flow](#architecture--data-flow)
13. [Optimisation Notes](#optimisation-notes)
14. [Tuning Guide](#tuning-guide)
15. [Troubleshooting](#troubleshooting)
16. [Privacy & Ethics](#privacy--ethics)
17. [Limitations](#limitations)
18. [License](#license)

---

## Quick Start

```
1.  Install Arduino IDE + ESP32 board support (see Software Setup)
2.  Open passive-wifi-crowd-counter.ino
3.  Upload to your ESP32
4.  Open Serial Monitor @ 115200 baud
5.  Type 'help' for available commands
```

---

## How It Works

Every smartphone and laptop with Wi-Fi enabled constantly broadcasts **Probe Request** frames — even when not connected to any network. These frames contain the device's source MAC address and are sent on multiple Wi-Fi channels.

This project:

1. Puts the ESP32's radio into **promiscuous mode** so it can hear all 802.11 traffic.
2. Captures raw Layer-2 frames via a driver callback (runs in ISR context).
3. Filters for **Probe Requests** (Frame Control subtype `0x04`).
4. Extracts the **source MAC** and **RSSI** (signal strength).
5. Tracks unique MACs in a fixed-size **hash table** with automatic expiry.
6. Reports a smoothed **people count** over serial every 5 seconds.
7. Optionally **categorises devices** by vendor (Phone / Laptop / IoT).

### Why Probe Requests?

| Frame Type | When Sent | Carries MAC? | Useful? |
|---|---|---|---|
| **Probe Request** | Constantly (Wi-Fi on) | Yes (Source) | **Best candidate** |
| Beacon | Only from APs | AP's MAC only | No (not from phones) |
| Data | Only when connected | Yes | No (requires association) |
| Auth/Assoc | Only during connection | Yes | Too rare |

---

## Hardware

### Minimum Requirements

| Component | Specification | Approx. Cost |
|---|---|---|
| **ESP32 DevKit** | ESP32-WROOM-32U (**external antenna**) | $3–5 |
| USB cable | Micro-USB or USB-C | $1 |
| Computer | For programming + serial monitor | — |

### Recommended

| Component | Why |
|---|---|
| **External antenna** (2.4 GHz, 3 dBi) | 2–3× range vs PCB antenna |
| **USB power bank** | Deploy standalone without a computer |
| **3D-printed / cardboard case** | Protect the board |

### ESP32-WROOM-32U vs WROOM-32

| Feature | WROOM-32 | WROOM-32U |
|---|---|---|
| Antenna | PCB trace | U.FL connector (external) |
| Range (indoor) | ~10–15 m | ~20–40 m |
| **Recommended** | Adequate for small rooms | **Yes** — better capture rate |

---

## Software Setup

### 1. Install Arduino IDE

Download from [arduino.cc/en/software](https://www.arduino.cc/en/software) (v2.x recommended).

### 2. Add ESP32 Board Support

1. **File → Preferences** → **Additional Boards Manager URLs**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. **Tools → Board → Boards Manager** → Search `esp32` → Install

### 3. Board Settings

| Setting | Value |
|---|---|
| Board | `ESP32 Dev Module` |
| Upload Speed | `921600` |
| Flash Frequency | `80 MHz` |
| Flash Mode | `QIO` |
| Flash Size | `4 MB (32 Mb)` |
| Partition Scheme | `Default 4MB with spiffs` |
| CPU Frequency | `240 MHz (WiFi)` |

### 4. Libraries

No external libraries needed. Uses only built-in ESP32 Arduino core libraries:
- `<Arduino.h>` — core API
- `<WiFi.h>` — WiFi driver
- `<esp_wifi.h>` — low-level promiscuous mode API

---

## Project Structure

```
passive-wifi-crowd-counter/
├── passive-wifi-crowd-counter.ino   # Main entry point (setup + loop)
├── config.h                          # All user-configurable constants
├── types.h                           # Shared data structures
├── hash_table.h / .cpp               # MAC deduplication (ISR-safe)
├── sniffer.h / .cpp                  # 802.11 packet parsing callback
├── channel_manager.h / .cpp          # Non-blocking channel hopper
├── stats.h / .cpp                    # Rolling average & peak tracker
├── serial_cli.h / .cpp               # Interactive serial commands
├── reporter.h / .cpp                 # Formatted serial output
├── oui_lookup.h / .cpp               # MAC vendor prefix matching
├── README.md                         # This file
└── LICENSE                           # MIT License
```

### Module Responsibilities

| Module | Responsibility | ISR-safe? |
|---|---|---|
| `config.h` | All `#define` constants | — |
| `types.h` | `DeviceSlot`, `CrowdStats` structs | — |
| `hash_table` | Open-addressing hash table for MAC dedup | **Yes** |
| `sniffer` | Packet parsing callback, RSSI filtering | **Yes** (ISR) |
| `channel_manager` | Non-blocking WiFi channel rotation | No |
| `stats` | Rolling average, peak tracking | No |
| `serial_cli` | Interactive command parser | No |
| `reporter` | Human-readable + CSV serial output | No |
| `oui_lookup` | MAC prefix → vendor name/category | No |

---

## Serial CLI Reference

Type commands into the Serial Monitor at runtime. No reboot required.

```
crowd> help
```

| Command | Description |
|---|---|
| `help` | Show all available commands |
| `count` | Print current crowd count |
| `peak` | Print peak count since boot |
| `avg` | Print rolling average |
| `status` | Full status dashboard (count, RSSI, channel, uptime, memory) |
| `channels` | Per-channel hit distribution with bar chart |
| `vendors` | Device vendor breakdown (Phone / Laptop / IoT / Unknown) |
| `rssi [value]` | Get or set RSSI threshold at runtime |
| `expiry [seconds]` | Get or set MAC expiry timeout |
| `dump` | List all tracked MACs with RSSI, age, and vendor |
| `reset` | Clear all tracked devices and statistics |
| `csv` | Toggle CSV output mode (for data logging) |
| `pause` | Pause scanning (stops counting) |
| `resume` | Resume scanning |
| `led` | Toggle LED heartbeat on/off |

### Example Session

```
crowd> status

  ┌──────────────────────────────────┐
  │      Crowd Counter  Status        │
  ├──────────────────────────────────┤
  │  Uptime      : 00:05:32              │
  │  Count       : 14 / 200              │
  │  Peak        : 23                    │
  │  Avg         : 12                    │
  │  RSSI thresh : -70 dBm               │
  │  Channel     : 8                     │
  │  Paused      : NO                    │
  │  CSV mode    : NO                    │
  └──────────────────────────────────┘

crowd> vendors
  Device vendor breakdown:
    Phone : 9
    Laptop : 3
    IoT : 1
    Unknown : 1

crowd> rssi -65
[rssi] Threshold set to -65 dBm

crowd> dump
  MAC                 RSSI    Age(s)  Vendor
  ──────────────────  ──────  ──────  ──────────
  A4:83:E7:12:BC:01   -54 dBm   3s    Apple
  3C:22:FB:AA:11:22   -62 dBm   7s    Apple
  ...
  Total: 14
```

---

## Configuration Reference

All constants are in `config.h`. No library edits needed.

### Scanning

| Constant | Default | Description |
|---|---|---|
| `CHANNEL_MIN` | `1` | Lowest WiFi channel to scan |
| `CHANNEL_MAX` | `13` | Highest WiFi channel to scan |
| `CHANNEL_HOP_MS` | `200` | Dwell time per channel (ms) |

### Signal Filter

| Constant | Default | Description |
|---|---|---|
| `RSSI_THRESHOLD_DEFAULT` | `-70` | Initial minimum signal strength |
| `RSSI_THRESHOLD_MIN` | `-90` | CLI lower bound |
| `RSSI_THRESHOLD_MAX` | `-30` | CLI upper bound |

### Device Tracking

| Constant | Default | Description |
|---|---|---|
| `MAC_EXPIRY_MS` | `300000` | Silent MAC eviction timeout (5 min) |
| `MAX_TRACKED_DEVICES` | `200` | Hard cap on unique MACs |
| `HASH_TABLE_SIZE` | `211` | Hash table buckets (prime, ~1.1× max) |

### Statistics

| Constant | Default | Description |
|---|---|---|
| `AVG_WINDOW` | `6` | Rolling average sample count |
| `REPORT_INTERVAL_MS` | `5000` | How often to print reports |

### Hardware

| Constant | Default | Description |
|---|---|---|
| `LED_PIN` | `2` | GPIO for heartbeat LED (-1 to disable) |
| `SERIAL_BAUD` | `115200` | Serial monitor baud rate |

### Feature Toggles

| Constant | Default | Description |
|---|---|---|
| `ENABLE_SERIAL_CLI` | `1` | Interactive serial commands |
| `ENABLE_CSV_LOGGING` | `1` | CSV output mode support |
| `ENABLE_OUI_LOOKUP` | `1` | MAC vendor prefix matching |
| `ENABLE_WEB_SERVER` | `0` | HTTP JSON API (see note below) |

### Optional: Web Server

When `ENABLE_WEB_SERVER = 1`, the ESP32 runs a lightweight HTTP server in APSTA mode alongside promiscuous mode. A JSON endpoint is available at `http://192.168.4.1/api/count`.

> **Trade-off:** This may slightly reduce capture performance because the radio time-slices between AP and STA interfaces. Enable only if you need programmatic access to the count.

---

## Serial Output Formats

### Human-Readable (default)

```
--------------------------------------------------
[00:05:32]  Crowd: 14  (avg: 12, peak: 23)  RSSI >= -70 dBm  ch: 8
  Slots: 14/200
```

| Field | Meaning |
|---|---|
| `00:05:32` | Uptime (HH:MM:SS) |
| `Crowd: 14` | Current unique MACs |
| `avg: 12` | Rolling average (last 6 reports) |
| `peak: 23` | Highest count since boot |
| `RSSI >= -70 dBm` | Active filter |
| `ch: 8` | Current scan channel |
| `Slots: 14/200` | Hash table capacity |

### CSV Mode (`csv` command)

```csv
timestamp_ms,count,peak,avg,rssi_thresh,channel,slots_used
332000,14,23,12,-70,8,14
337000,16,23,13,-70,3,16
```

Pipe the serial output to a file for data collection:

```bash
# Linux / macOS
python -m serial.tools.miniterm /dev/ttyUSB0 115200 | tee crowd_data.csv

# Windows (PowerShell)
python -m serial.tools.miniterm COM3 115200 | Out-File crowd_data.csv
```

---

## OUI Vendor Lookup

The first 3 bytes of a MAC address identify the hardware manufacturer (OUI — Organisationally Unique Identifier). The built-in table categorises devices:

| Category | Vendors Included | Typical Devices |
|---|---|---|
| **Phone** | Apple, Samsung, Google | iPhones, Galaxy, Pixel |
| **Laptop** | Intel, Realtek, Broadcom | Wi-Fi adapters in PCs |
| **IoT** | Espressif, Tuya, Xiaomi | Smart home devices |
| **Unknown** | Not in table | Everything else |

Use the `vendors` CLI command to see a live breakdown:

```
crowd> vendors
  Device vendor breakdown:
    Phone : 9
    Laptop : 3
    IoT : 1
    Unknown : 1
```

### Adding Custom OUI Entries

Edit `oui_lookup.cpp` and add entries to `g_ouiTable[]`:

```cpp
{ {0xAA, 0xBB, 0xCC}, "MyVendor", VENDOR_PHONE },
```

Then update `OUI_TABLE_SIZE` in `config.h`.

---

## CSV Data Logging

1. Set the serial CLI to CSV mode:
   ```
   crowd> csv
   [csv] Output mode: CSV
   ```

2. The header line is printed immediately:
   ```csv
   timestamp_ms,count,peak,avg,rssi_thresh,channel,slots_used
   ```

3. Every 5 seconds, a new row is appended.

4. Capture the serial output to a file (see example commands above).

5. Open in Excel, Google Sheets, or any data analysis tool.

---

## 802.11 Header Parsing

This is the most technical part of the code. Here's the byte-by-byte breakdown:

### Raw Frame Structure

```
Byte:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18
     ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
     │ FC0 │ FC1 │DurID│        Address 1 (Dest)       │       Address 2 (Source)       │      Address 3 (BSSID)     │
     └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
             ↑                                                           ↑
          Frame                                                      Source MAC
          Control                                                    (we extract this)
```

### Frame Control Byte 0 (FC0)

```
Bit:    7     6     5     4     3     2     1     0
     ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
     │  Subtype (4 bits)  │ Type │ Proto│ Proto│     │
     │  └── 0x04 = Probe  │(2bit)│ ver  │ ver  │     │
     └────────────────────┴──────┴──────┴──────┴─────┘
              bits[7:4]      bits[3:2]   bits[1:0]
```

**Code:**
```cpp
uint8_t subtype = (frame[0] >> 4) & 0x0F;  // → 0x04 for Probe Request
const uint8_t *srcMac = &frame[10];          // Address 2 at offset 10
int8_t rssi = pkt->rx_ctrl.rssi;             // from driver metadata
```

### Why RSSI Is Not in the Frame

The RSSI (Received Signal Strength Indicator) is a measurement made by the radio hardware. It is attached to the frame by the ESP32 driver in a separate metadata struct (`wifi_promiscuous_pkt_t.rx_ctrl`). It is never part of the 802.11 frame itself.

---

## Architecture & Data Flow

```
  ┌─────────────────────────────────────────────────────┐
  │                 ESP32 Wi-Fi Radio                    │
  │              (Promiscuous Mode)                      │
  └──────────────────────┬──────────────────────────────┘
                         │ raw 802.11 frame + metadata
                         ▼
  ┌──────────────────────────────────────────────────────┐
  │          sniffer.cpp — snifferCallback() [ISR]       │
  │                                                      │
  │  1. type == WIFI_PKT_MGMT ?                          │
  │  2. subtype == 0x04 ?  (Probe Request)               │
  │  3. rssi >= threshold ?                              │
  │  4. hashTableUpsert(mac, rssi)                       │
  │  5. channelManagerHits[ch]++                         │
  └──────────────────────┬──────────────────────────────┘
                         │
                         ▼
  ┌──────────────────────────────────────────────────────┐
  │       hash_table.cpp — g_table[211] (fixed)          │
  │                                                      │
  │  ┌────────┬─────────┬────────────┬──────────┐        │
  │  │ mac[6] │ rssi    │ lastSeenMs │ occupied │        │
  │  └────────┴─────────┴────────────┴──────────┘        │
  └──────────────────────┬──────────────────────────────┘
                         │
                         ▼
  ┌──────────────────────────────────────────────────────┐
  │               passive-wifi-crowd-counter.ino          │
  │                        loop()                         │
  │                                                      │
  │  channelManagerHop()  → rotate ch 1–13               │
  │  expireTick()         → evict stale MACs             │
  │  reportTick()         → build stats, print output    │
  │  cliPoll()            → process serial commands      │
  │  cliLedHeartbeat()    → 1 Hz LED blink              │
  └──────────────────────────────────────────────────────┘
```

---

## Optimisation Notes

### v1 → v2 → v3 Improvements

| Area | v1 | v2 | v3 |
|---|---|---|---|
| **Architecture** | Single file | Single file | **10 modular files** |
| **MAC storage** | `std::map<string>` | Fixed hash table | Fixed hash table |
| **ISR allocations** | Heap (dangerous) | Zero | Zero |
| **CLI** | None | None | **Interactive 15-command CLI** |
| **Vendor lookup** | None | None | **OUI prefix matching** |
| **CSV output** | None | None | **Toggle-able CSV mode** |
| **Stats** | Raw count | Rolling avg + peak | Rolling avg + peak + channels |
| **Config** | In-code defines | In-code defines | **Dedicated config.h** |
| **Runtime tuning** | Reboot required | Reboot required | **CLI: rssi, expiry, reset** |

### Why a Hash Table Instead of `std::map`?

The sniffer callback runs in **ISR context**:

- **Heap allocation is forbidden** (causes crashes or fragmentation)
- **CPU time is precious** (blocks other interrupts)
- `std::map::operator[]` may allocate → **deadly in ISR**
- `std::string` allocates heap → **deadly in ISR**

The fixed hash table (open-addressing, FNV-1a) needs zero dynamic allocation and completes in O(1) amortised time.

### IRAM_ATTR

Places the callback in Instruction RAM instead of Flash. Critical because:
- Flash reads during interrupts can crash (shared with cache)
- IRAM is always accessible → deterministic timing

---

## Tuning Guide

### Small Conference Room (10–30 people)

```cpp
#define RSSI_THRESHOLD_DEFAULT  (-65)
#define MAC_EXPIRY_MS           180000UL
#define MAX_TRACKED_DEVICES     50
#define CHANNEL_HOP_MS          150
```

Or at runtime:
```
crowd> rssi -65
crowd> expiry 180
```

### Large Lecture Hall (50–200 people)

```cpp
#define RSSI_THRESHOLD_DEFAULT  (-75)
#define MAC_EXPIRY_MS           300000UL
#define MAX_TRACKED_DEVICES     200
#define CHANNEL_HOP_MS          250
```

### Building Entrance (pass-through counting)

```cpp
#define RSSI_THRESHOLD_DEFAULT  (-55)
#define MAC_EXPIRY_MS           60000UL
#define MAX_TRACKED_DEVICES     100
```

### Quick Reference

| Problem | Solution |
|---|---|
| Count too high | Raise threshold: `crowd> rssi -60` |
| Count too low | Lower threshold: `crowd> rssi -80` |
| Count fluctuates | Increase `AVG_WINDOW` in config.h |
| RAM crashes | Lower `MAX_TRACKED_DEVICES` |
| No captures | Add external antenna, lower threshold |

---

## Troubleshooting

### "Count is always 0"

1. Check board variant — must be ESP32 (not S2/S3/C3 in某些 modes).
2. Check antenna — WROOM-32U needs external antenna on U.FL.
3. Try `crowd> rssi -80` to lower the threshold.
4. Verify baud rate is 115200.

### "Count is unrealistically high"

1. Raise threshold: `crowd> rssi -60`.
2. Lower expiry: `crowd> expiry 120`.
3. MAC randomisation — modern phones randomise MACs; one person → multiple MACs.

### "Serial output is garbled"

1. Baud rate must be **115200** in Serial Monitor.
2. Press **RST** on the ESP32 after opening Serial Monitor.
3. Check `SERIAL_BAUD` matches.

### "Board crashes or hangs"

1. Lower `MAX_TRACKED_DEVICES` — each slot is 12 bytes.
2. Keep `CHANNEL_HOP_MS` ≥ 100.
3. Try `crowd> reset` to clear the table.

### Upload Fails

1. Hold **BOOT** button while IDE says "Connecting...".
2. Try a different USB cable (some are power-only).
3. Reduce upload speed to `115200`.

---

## Privacy & Ethics

### What This Device Does

- Captures **randomised MAC addresses** broadcast publicly over radio
- Does **not** decrypt any traffic
- Does **not** connect to any network
- Does **not** store or transmit data externally
- All data is lost on power-off

### What This Device Does NOT Do

- Does **not** identify individuals
- Does **not** track location over time
- Does **not** intercept communications
- Does **not** require cooperation from tracked devices

### Important Notes

- **MAC randomisation**: iOS 14+ and Android 10+ randomise Wi-Fi MACs. The counter measures *device appearances*, not *people*.
- **Consent**: Passive radio monitoring may be subject to surveillance laws in some jurisdictions.
- **Best use**: Relative occupancy estimator (trend), not an exact headcount.

---

## Limitations

| Limitation | Impact | Mitigation |
|---|---|---|
| MAC randomisation | 1 person → multiple MACs | Use as relative trend |
| Wi-Fi off | Device invisible | Cannot solve passively |
| Environment variance | Walls/furniture attenuate signal | Tune RSSI, use external antenna |
| Single antenna | No direction finding | Deploy multiple units |
| 2.4 GHz only | 5 GHz devices not scanned | ESP32 limitation |
| No persistent storage | Count resets on reboot | Add SD logging (not included) |

---

## License

MIT License. See [LICENSE](LICENSE).

---

## Credits

- Espressif ESP32 Arduino Core: [github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
- IEEE 802.11-2020 specification
- Inspired by: [SpacehuhnTech/esp8266_deauther](https://github.com/SpacehuhnTech/esp8266_deauther), [schneems/SwarmSC](https://github.com/schneems/SwarmSC)
