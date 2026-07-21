# Passive WiFi Crowd Counter — ESP32

A **zero-cost, privacy-respecting** people counter that estimates room occupancy by passively sniffing Wi-Fi probe requests. No cameras, no beacons, no cloud — just a $3 ESP32 board and radio physics.

> **v3.1** — ESP-IDF CMake build, pure C, modular architecture, serial CLI, OUI vendor lookup, CSV logging. ISR-safe concurrency, fixed channel reporting.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites](#prerequisites)
3. [How It Works](#how-it-works)
4. [Hardware](#hardware)
5. [Project Structure](#project-structure)
6. [Building & Flashing](#building--flashing)
7. [Serial CLI Reference](#serial-cli-reference)
8. [Configuration Reference](#configuration-reference)
9. [Serial Output Formats](#serial-output-formats)
10. [OUI Vendor Lookup](#oui-vendor-lookup)
11. [802.11 Header Parsing](#80211-header-parsing)
12. [Architecture & Data Flow](#architecture--data-flow)
13. [Tuning Guide](#tuning-guide)
14. [Troubleshooting](#troubleshooting)
15. [Privacy & Ethics](#privacy--ethics)
16. [License](#license)

---

## Quick Start

```bash
# Clone the repo
git clone https://github.com/stalss/passive-wifi-crowd-counter.git
cd passive-wifi-crowd-counter

# Set up ESP-IDF environment (see Prerequisites)
. $IDF_PATH/export.sh

# Build, flash, and monitor
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Type `help` in the monitor for a list of commands.

---

## Prerequisites

### ESP-IDF v5.1+

This project uses the **ESP-IDF** build system (CMake). It does **not** use the Arduino framework.

1. **Install ESP-IDF v5.1+** following the official guide:
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

2. **Set up the environment** in every new terminal:
   ```bash
   . $IDF_PATH/export.sh
   ```

3. **Verify** by running:
   ```bash
   idf.py --version
   # Should print v5.1 or later
   ```

### Toolchain

ESP-IDF bundles its own GCC toolchain — no separate installation needed. The CMake build system handles everything automatically.

---

## How It Works

Every smartphone and laptop with Wi-Fi enabled constantly broadcasts **Probe Request** frames — even when not connected to any network. These frames contain the device's source MAC address and are sent on multiple Wi-Fi channels.

This project:

1. Puts the ESP32's radio into **promiscuous mode** so it can hear all 802.11 traffic.
2. Captures raw Layer-2 frames via a driver callback (runs in ISR context).
3. Filters for **Probe Requests** (Frame Control subtype `0x04`).
4. Extracts the **source MAC** and **RSSI** (signal strength).
5. Tracks unique MACs in a fixed-size **hash table** with automatic expiry.
6. Reports a smoothed **people count** over UART every 5 seconds.
7. Optionally **categorises devices** by vendor (Phone / Laptop / IoT).

### Why Probe Requests?

| Frame Type | When Sent | Carries MAC? | Useful? |
|---|---|---|---|
| **Probe Request** | Constantly (Wi-Fi on) | Yes (Source) | **Best candidate** |
| Beacon | Only from APs | AP's MAC only | No |
| Data | Only when connected | Yes | No |
| Auth/Assoc | Only during connection | Yes | Too rare |

---

## Hardware

### Minimum Requirements

| Component | Specification | Approx. Cost |
|---|---|---|
| **ESP32 DevKit** | ESP32-WROOM-32U (external antenna) | $3–5 |
| USB cable | Micro-USB or USB-C | $1 |

### Recommended

| Component | Why |
|---|---|
| **External antenna** (2.4 GHz, 3 dBi) | 2–3× range vs PCB antenna |
| **USB power bank** | Deploy standalone |

### ESP32-WROOM-32U vs WROOM-32

| Feature | WROOM-32 | WROOM-32U |
|---|---|---|
| Antenna | PCB trace | U.FL connector |
| Range (indoor) | ~10–15 m | ~20–40 m |
| **Recommended** | Adequate | **Yes** |

---

## Project Structure

```
passive-wifi-crowd-counter/
├── CMakeLists.txt              # Top-level ESP-IDF CMake
├── main/
│   ├── CMakeLists.txt          # Component CMake (sources + deps)
│   ├── app_main.c              # Entry point + FreeRTOS tasks
│   ├── config.h                # All user-configurable constants
│   ├── types.h                 # Shared data structures
│   ├── hash_table.h / .c       # MAC deduplication (ISR-safe)
│   ├── sniffer.h / .c          # 802.11 packet parsing callback
│   ├── channel_manager.h / .c  # Non-blocking channel hopper
│   ├── stats.h / .c            # Rolling average & peak tracker
│   ├── serial_cli.h / .c       # Interactive CLI (esp_console)
│   ├── reporter.h / .c         # Formatted serial output
│   └── oui_lookup.h / .c       # MAC vendor prefix matching
├── README.md
└── LICENSE
```

### Module Responsibilities

| Module | Responsibility | Runs in |
|---|---|---|
| `config.h` | All `#define` constants | — |
| `types.h` | `DeviceSlot`, `CrowdStats` structs | — |
| `hash_table` | Open-addressing hash table | ISR + tasks |
| `sniffer` | Packet parsing callback | **ISR** |
| `channel_manager` | WiFi channel rotation | Task |
| `stats` | Rolling average, peak tracking | Task |
| `serial_cli` | `esp_console` REPL + 15 commands | Task |
| `reporter` | Human-readable + CSV output | Task |
| `oui_lookup` | MAC prefix → vendor/category | Task |

---

## Building & Flashing

### First-Time Setup

```bash
# 1. Install ESP-IDF (if not done)
mkdir -p ~/esp
cd ~/esp
git clone -b v5.1.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
. ./export.sh

# 2. Clone this project
git clone https://github.com/stalss/passive-wifi-crowd-counter.git
cd passive-wifi-crowd-counter
```

### Build

```bash
idf.py build
```

### Flash & Monitor

```bash
# Replace /dev/ttyUSB0 with your port
idf.py -p /dev/ttyUSB0 flash monitor
```

### Build Only (no flash)

```bash
idf.py build
```

### Clean Build

```bash
idf.py fullclean
idf.py build
```

### Set Target (if needed)

```bash
idf.py set-target esp32
idf.py build
```

### Monitor Only (after flash)

```bash
idf.py -p /dev/ttyUSB0 monitor
```

### Exit Monitor

Press `Ctrl+]` to exit the IDF monitor.

---

## Serial CLI Reference

The CLI uses ESP-IDF's `esp_console` with a full REPL — line editing, tab completion, and command history are built in.

Type `help` for the full list:

```
ESP32> help

  Available commands:
  ─────────────────────────────────────
  help               Show this message
  count              Current crowd count
  peak               Peak count since boot
  avg                Rolling average
  status             Full status report
  channels           Per-channel hit counts
  vendors            Device vendor breakdown
  rssi [value]       Get/set RSSI threshold
  expiry [seconds]   Get/set MAC expiry
  dump               List all tracked MACs
  reset              Clear all devices
  csv                Toggle CSV output
  pause              Pause scanning
  resume             Resume scanning
  led                Toggle LED heartbeat
```

### Example Session

```
ESP32> status

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
  │  LED         : ON                    │
  └──────────────────────────────────┘

ESP32> vendors
  Device vendor breakdown:
    Phone : 9
    Laptop : 3
    IoT : 1
    Unknown : 1

ESP32> rssi -65
[rssi] Threshold set to -65 dBm

ESP32> dump
  MAC                 RSSI    Age(s)  Vendor
  ──────────────────  ──────  ──────  ──────────
  A4:83:E7:12:BC:01   -54 dBm   3s    Apple
  3C:22:FB:AA:11:22   -62 dBm   7s    Apple
  ...
  Total: 14

ESP32> csv
[csv] Output mode: CSV

I (12345) reporter: timestamp_ms,count,peak,avg,rssi_thresh,channel,slots_used
I (17456) reporter: 5000,14,23,12,-70,8,14
```

---

## Configuration Reference

All constants are in `main/config.h`. Rebuild after changes.

### Scanning

| Constant | Default | Description |
|---|---|---|
| `CHANNEL_MIN` | `1` | Lowest WiFi channel |
| `CHANNEL_MAX` | `13` | Highest WiFi channel |
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
| `MAC_EXPIRY_MS` | `300000` | Silent MAC eviction (5 min) |
| `MAX_TRACKED_DEVICES` | `200` | Hard cap on unique MACs |
| `HASH_TABLE_SIZE` | `211` | Hash table buckets (prime) |

### Statistics

| Constant | Default | Description |
|---|---|---|
| `AVG_WINDOW` | `6` | Rolling average sample count |
| `REPORT_INTERVAL_MS` | `5000` | Report interval |

### Hardware

| Constant | Default | Description |
|---|---|---|
| `LED_PIN` | `2` | GPIO for heartbeat LED (-1 to disable) |

### Task Configuration

| Constant | Default | Description |
|---|---|---|
| `TASK_STACK_SIZE_CHANNEL` | `4096` | Channel hopper stack |
| `TASK_STACK_SIZE_REPORT` | `4096` | Reporter stack |
| `TASK_STACK_SIZE_CLI` | `8192` | CLI REPL stack |
| `TASK_STACK_SIZE_EXPIRE` | `4096` | Expiry task stack |
| `TASK_PRIO_CHANNEL` | `5` | Channel hopper priority |
| `TASK_PRIO_REPORT` | `3` | Reporter priority |
| `TASK_PRIO_CLI` | `2` | CLI priority |
| `TASK_PRIO_EXPIRE` | `3` | Expiry priority |

---

## Serial Output Formats

### Human-Readable (default)

```
--------------------------------------------------
[00:05:32]  Crowd: 14  (avg: 12, peak: 23)  RSSI >= -70 dBm  ch: 8
  Slots: 14/200
```

### CSV Mode

```csv
timestamp_ms,count,peak,avg,rssi_thresh,channel,slots_used
5000,14,23,12,-70,8,14
10000,16,23,13,-70,3,16
```

---

## OUI Vendor Lookup

The first 3 bytes of a MAC address identify the hardware manufacturer. The built-in table categorises devices:

| Category | Vendors | Typical Devices |
|---|---|---|
| **Phone** | Apple, Samsung, Google | iPhones, Galaxy, Pixel |
| **Laptop** | Intel, Realtek, Broadcom | Wi-Fi adapters |
| **IoT** | Espressif, Tuya, Xiaomi | Smart home devices |
| **Unknown** | Not in table | Everything else |

Use `vendors` in the CLI for a live breakdown.

### Adding Entries

Edit `main/oui_lookup.c`, append to `g_ouiTable[]`, and update `OUI_TABLE_SIZE` in `config.h`.

---

## 802.11 Header Parsing

### Frame Layout

```
Byte:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17
     ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
     │ FC0 │ FC1 │DurID│       Address 1 (Dest)        │       Address 2 (Source)       │    Address 3     │
     └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
             ↑                                                         ↑
          Frame                                                     Source MAC
          Control                                                   (extract this)
```

### Code

```c
/* Subtype: byte 0, bits [7:4] */
uint8_t subtype = (frame[0] >> 4) & 0x0F;
if (subtype != 0x04) return;  /* not Probe Request */

/* Source MAC at byte offset 10 */
const uint8_t *srcMac = &frame[10];

/* RSSI from driver metadata (NOT in the frame) */
int8_t rssi = pkt->rx_ctrl.rssi;
```

---

## Architecture & Data Flow

```
  ┌─────────────────────────────────────────────────────┐
  │                 ESP32 Wi-Fi Radio                    │
  │              (Promiscuous Mode)                      │
  └──────────────────────┬──────────────────────────────┘
                         │ 802.11 frame + rx_ctrl metadata
                         ▼
  ┌──────────────────────────────────────────────────────┐
  │         sniffer.c — snifferCallback() [ISR]          │
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
  │      hash_table.c — g_table[211] (fixed, ISR-safe)   │
  │                                                      │
  │  ┌────────┬─────────┬────────────┬──────────┐        │
  │  │ mac[6] │ rssi    │ lastSeenMs │ occupied │        │
  │  └────────┴─────────┴────────────┴──────────┘        │
  └──────────────────────┬──────────────────────────────┘
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
  ┌──────────────┐ ┌──────────┐ ┌──────────────┐
  │ channelTask  │ │expireTask│ │ reportTask   │
  │ (hop ch 1–13)│ │(evict    │ │ (print stats │
  │              │ │ stale)   │ │  every 5s)   │
  └──────────────┘ └──────────┘ └──────────────┘
                         │
                         ▼
                 ┌──────────────┐
                 │   cliTask    │
                 │ (esp_console │
                 │  REPL on UART)│
                 └──────────────┘
```

### FreeRTOS Task Layout

| Task | Priority | Stack | Function |
|---|---|---|---|
| `channelTask` | 5 (highest) | 4 KB | Hops WiFi channels every 200 ms |
| `expireTask` | 3 | 4 KB | Evicts stale MACs every 1 s |
| `reportTask` | 3 | 4 KB | Prints crowd stats every 5 s |
| `cliTask` | 2 (lowest) | 8 KB | Runs `esp_console` REPL |
| `ledTask` | 1 | 2 KB | 1 Hz heartbeat LED blink |

---

## Tuning Guide

### Small Conference Room (10–30 people)

```c
#define RSSI_THRESHOLD_DEFAULT  (-65)
#define MAC_EXPIRY_MS           180000UL
#define MAX_TRACKED_DEVICES     50
```

Or at runtime: `rssi -65` then `expiry 180`

### Large Lecture Hall (50–200 people)

```c
#define RSSI_THRESHOLD_DEFAULT  (-75)
#define MAC_EXPIRY_MS           300000UL
#define MAX_TRACKED_DEVICES     200
```

### Building Entrance

```c
#define RSSI_THRESHOLD_DEFAULT  (-55)
#define MAC_EXPIRY_MS           60000UL
#define MAX_TRACKED_DEVICES     100
```

### Quick Reference

| Problem | Solution |
|---|---|
| Count too high | `rssi -60` |
| Count too low | `rssi -80` or add external antenna |
| Count fluctuates | Increase `AVG_WINDOW` in config.h |
| RAM crashes | Lower `MAX_TRACKED_DEVICES` |

---

## Troubleshooting

### Build fails

1. Ensure ESP-IDF v5.1+ is installed and sourced (`. $IDF_PATH/export.sh`)
2. Run `idf.py set-target esp32` before building
3. Try `idf.py fullclean && idf.py build`

### Flash fails

1. Hold **BOOT** button while idf.py says "Connecting..."
2. Check port: `ls /dev/ttyUSB*` (Linux) or Device Manager (Windows)
3. Try lower speed: `idf.py -p /dev/ttyUSB0 -b 115200 flash`

### Count is always 0

1. Check antenna — WROOM-32U needs external antenna on U.FL
2. Try `rssi -80` to lower the threshold
3. Ensure you're using ESP32 (not S2/S3/C3)

### Count is unrealistically high

1. Raise threshold: `rssi -60`
2. Lower expiry: `expiry 120`
3. MAC randomisation — one person may appear as multiple MACs

### Board crashes

1. Lower `MAX_TRACKED_DEVICES`
2. Keep `CHANNEL_HOP_MS` ≥ 100
3. `reset` to clear the table

---

## Privacy & Ethics

### What This Device Does

- Captures **randomised MAC addresses** broadcast publicly over radio
- Does **not** decrypt any traffic
- Does **not** connect to any network
- Does **not** store or transmit data externally
- All data is lost on power-off

### What It Does NOT Do

- Does **not** identify individuals
- Does **not** track location over time
- Does **not** intercept communications
- Does **not** require cooperation from tracked devices

### Important Notes

- **MAC randomisation**: iOS 14+ and Android 10+ randomise MACs. One person → multiple MACs.
- **Consent**: Check local regulations before deployment.
- **Best use**: Relative occupancy estimator, not exact headcount.

---

## License

MIT License. See [LICENSE](LICENSE).

---

## Credits

- Espressif ESP-IDF: [github.com/espressif/esp-idf](https://github.com/espressif/esp-idf)
- IEEE 802.11-2020
- Inspired by: [SpacehuhnTech/esp8266_deauther](https://github.com/SpacehuhnTech/esp8266_deauther)
