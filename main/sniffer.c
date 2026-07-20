#include "sniffer.h"
#include "config.h"
#include "hash_table.h"
#include "channel_manager.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include <string.h>

/* ================================================================
 *  802.11 Sniffer Callback (ISR context)
 *
 *  Frame Control byte 0 layout:
 *    bits[7:4] = Subtype  (0x04 = Probe Request)
 *    bits[3:2] = Type     (0 = Management)
 *    bits[1:0] = Protocol (always 0)
 *
 *  Management frame header offsets:
 *    Byte  0–1  Frame Control
 *    Byte  2–3  Duration / ID
 *    Byte  4–9  Address 1 (Destination)
 *    Byte 10–15 Address 2 (Source)  ← extract this
 *    Byte 16–21 Address 3 (BSSID)
 *
 *  RSSI is NOT in the frame — it's in the driver metadata
 *  (wifi_promiscuous_pkt_t.rx_ctrl.rssi).
 * ================================================================ */

static const char *TAG = "sniffer";

static int g_rssiThreshold = RSSI_THRESHOLD_DEFAULT;
static volatile int g_paused = 0;

static void IRAM_ATTR snifferCallback(void *buf,
                                      wifi_promiscuous_pkt_type_t type)
{
    if (g_paused) return;
    if (type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    const uint8_t *frame = pkt->payload;

    /* Subtype extraction: byte 0, bits [7:4] */
    uint8_t subtype = (frame[0] >> 4) & 0x0F;
    if (subtype != 0x04) return; /* not Probe Request */

    /* Source MAC at byte offset 10 (Address 2) */
    const uint8_t *srcMac = &frame[10];

    /* RSSI from driver metadata */
    int8_t rssi = pkt->rx_ctrl.rssi;
    if (rssi < g_rssiThreshold) return;

    /* Channel hit counter */
    uint8_t ch = pkt->rx_ctrl.channel;
    if (ch <= CHANNEL_MAX) {
        channelManagerHits()[ch]++;
    }

    /* Hash table upsert */
    int isNew = hashTableUpsert(srcMac, rssi);

    /* LED flash on new device */
    if (isNew && LED_PIN >= 0) {
        gpio_set_level(LED_PIN, 1);
    }
}

/* ── Public API ─────────────────────────────────────────────────── */

void snifferInit(void) {
    /* Configure LED pin */
    if (LED_PIN >= 0) {
        gpio_reset_pin(LED_PIN);
        gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(LED_PIN, 0);
    }

    /* WiFi init — station mode required before promiscuous */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Enter promiscuous mode */
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(snifferCallback));

    /* Accept all management frames at driver level */
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(false));

    /* Set initial channel */
    ESP_ERROR_CHECK(esp_wifi_set_channel(CHANNEL_MIN, WIFI_SECOND_CHAN_NONE));

    ESP_LOGI(TAG, "Sniffer initialised (RSSI threshold: %d dBm)", g_rssiThreshold);
}

int snifferGetRssiThreshold(void) {
    return g_rssiThreshold;
}

void snifferSetRssiThreshold(int value) {
    g_rssiThreshold = value;
}

void snifferPause(void) {
    g_paused = 1;
}

void snifferResume(void) {
    g_paused = 0;
}

int snifferIsPaused(void) {
    return g_paused;
}
