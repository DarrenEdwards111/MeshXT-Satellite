/**
 * MeshXT-Satellite Gateway Configuration
 * © Mikoshi Ltd. — Apache 2.0
 */

#ifndef MESHXT_SATELLITE_CONFIG_H
#define MESHXT_SATELLITE_CONFIG_H

#include <stdint.h>

// ============================================================
// LoRaWAN Credentials (Lacuna Space)
// Replace with your registered device credentials
// ============================================================
#define LORAWAN_DEVEUI   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APPEUI   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APPKEY   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// Join mode: true = OTAA, false = ABP
#define LORAWAN_USE_OTAA true

// ABP credentials (only if OTAA is false)
#define LORAWAN_DEVADDR  0x00000000
#define LORAWAN_NWKSKEY  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APPSKEY  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// ============================================================
// LoRaWAN Radio Parameters
// ============================================================
#define LORAWAN_REGION_EU868  0
#define LORAWAN_REGION_US915  1
#define LORAWAN_REGION        LORAWAN_REGION_EU868

#define LORAWAN_SF            9        // Spreading factor (7-12)
#define LORAWAN_TX_POWER      14       // dBm
#define LORAWAN_FPORT         42       // Application port for MeshXT relay

// Duty cycle (EU868: 1% = 36000ms per hour)
#define DUTY_CYCLE_LIMIT_MS   36000
#define DUTY_CYCLE_WINDOW_MS  3600000  // 1 hour

// ============================================================
// Meshtastic Radio Parameters
// ============================================================

// Regional frequency presets (MHz)
#define MESHTASTIC_FREQ_EU868   869.525f
#define MESHTASTIC_FREQ_US915   906.875f
#define MESHTASTIC_FREQ_ANZ915  917.0f
#define MESHTASTIC_FREQ_CN470   470.0f

// Active region
#define MESHTASTIC_FREQUENCY    MESHTASTIC_FREQ_EU868

#define MESHTASTIC_SF           11       // Meshtastic long-range default
#define MESHTASTIC_BW           250000   // 250 kHz bandwidth
#define MESHTASTIC_CR           5        // Coding rate 4/5
#define MESHTASTIC_SYNC_WORD    0x2B     // Meshtastic sync word
#define MESHTASTIC_PREAMBLE     16       // Preamble length
#define MESHTASTIC_TX_POWER     17       // dBm

// Meshtastic channel key (AES-256, 32 bytes)
// Default channel or your custom key
#define MESHTASTIC_CHANNEL_KEY  { 0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59, \
                                  0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01, \
                                  0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59, \
                                  0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01 }

// ============================================================
// Hardware Pin Assignments (ESP32)
// ============================================================

// Radio 1 — Meshtastic (SX1276)
#define RADIO1_CS     18
#define RADIO1_RST    14
#define RADIO1_IRQ    26
#define RADIO1_BUSY   -1  // SX1276 has no BUSY pin

// Radio 2 — LoRaWAN (SX1262)
#define RADIO2_CS     5
#define RADIO2_RST    27
#define RADIO2_IRQ    33
#define RADIO2_BUSY   32

// SPI (shared bus)
#define SPI_SCK       12
#define SPI_MISO      13
#define SPI_MOSI      15

// Status LED
#define LED_PIN       25

// ============================================================
// Satellite Pass Scheduling
// ============================================================
#define SAT_PASS_INTERVAL_MS    5400000  // ~90 minutes between passes
#define SAT_PASS_DURATION_MS    600000   // ~10 minute pass window
#define SAT_PASS_WAKE_EARLY_MS  60000    // Wake 60s before predicted pass
#define SAT_USE_TLE             false    // Use TLE prediction (requires GPS + time)

// ============================================================
// Message Queue
// ============================================================
#define QUEUE_MAX_ENTRIES       64
#define QUEUE_MAX_PAYLOAD       128      // Max bytes per satellite payload

// TTL defaults (seconds)
#define TTL_EMERGENCY           86400    // 24 hours
#define TTL_HIGH                43200    // 12 hours
#define TTL_NORMAL              21600    // 6 hours
#define TTL_LOW                 7200     // 2 hours

// ============================================================
// MeshXT Compression / FEC
// ============================================================
#define MESHXT_COMPRESSION_ENABLED  true
#define MESHXT_FEC_ENABLED          true
#define MESHXT_FEC_REDUNDANCY       4    // Reed-Solomon parity symbols

// ============================================================
// Debug
// ============================================================
#define DEBUG_SERIAL            true
#define DEBUG_BAUD              115200

#endif // MESHXT_SATELLITE_CONFIG_H
