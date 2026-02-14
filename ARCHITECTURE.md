# Architecture — MeshXT-Satellite

## Overview

MeshXT-Satellite bridges two distinct radio protocols: Meshtastic (mesh LoRa) and LoRaWAN (satellite uplink). The gateway runs on dual-radio hardware, receiving on one frequency plan and transmitting on another.

## Packet Flow

### Uplink (Mesh → Satellite)

```
┌─────────────────┐     LoRa 915/868     ┌──────────────────────┐
│ Meshtastic Node  │ ──────────────────► │  MeshtasticReceiver   │
│ (MeshXT enabled) │                      │  (Radio 1 - SX1276)  │
└─────────────────┘                      └──────────┬───────────┘
                                                     │
                                                     ▼
                                          ┌──────────────────────┐
                                          │   PacketTranslator    │
                                          │  - Extract payload    │
                                          │  - Decompress MeshXT  │
                                          │  - Re-compress for    │
                                          │    satellite frame    │
                                          │  - Add routing header │
                                          └──────────┬───────────┘
                                                     │
                                                     ▼
                                          ┌──────────────────────┐
                                          │   Message Queue       │
                                          │  (Store & Forward)    │
                                          │  - Priority sorting   │
                                          │  - Deduplication      │
                                          │  - TTL management     │
                                          └──────────┬───────────┘
                                                     │
                                          (wait for satellite pass)
                                                     │
                                                     ▼
                                          ┌──────────────────────┐     LoRaWAN     ┌─────────────┐
                                          │  LoRaWANTransmitter   │ ─────────────► │   Lacuna     │
                                          │  (Radio 2 - SX1262)   │                │  Satellite   │
                                          └──────────────────────┘                └─────────────┘
```

### Downlink (Satellite → Mesh)

The reverse path: LoRaWAN downlink received on Radio 2, translated by PacketTranslator, and injected into the local Meshtastic mesh via Radio 1.

## Meshtastic → LoRaWAN Translation

### Meshtastic Packet Structure

```
┌────────────┬──────────┬──────────┬─────────┬──────────────┐
│ Dest (4B)  │ Src (4B) │ ID (4B)  │ Flags   │ Payload      │
│            │          │          │ (1B)    │ (up to 237B) │
└────────────┴──────────┴──────────┴─────────┴──────────────┘
```

For MeshXT packets, the payload contains:
```
┌──────────┬─────────┬──────────────────────┐
│ MeshXT   │ FEC     │ Compressed Message   │
│ Header   │ Parity  │                      │
│ (2B)     │ (var)   │ (var)                │
└──────────┴─────────┴──────────────────────┘
```

### Satellite Relay Packet (LoRaWAN Payload)

```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────────────┐
│ Version  │ Src Node │ Dest     │ Channel  │ Timestamp│ MeshXT Payload   │
│ (1B)     │ (4B)     │ (4B)     │ (1B)     │ (4B)     │ (var)            │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────────────┘
```

Total overhead: 14 bytes. With MeshXT compression, a typical text message of 50 characters compresses to ~30 bytes, giving a total satellite packet of ~44 bytes — well within LoRaWAN limits.

## MeshXT Header Preservation

The MeshXT compression header is preserved end-to-end. The satellite relay packet wraps the already-compressed MeshXT payload without re-compressing. This means:

1. Source node compresses with MeshXT
2. Gateway extracts the compressed payload (does not decompress)
3. Gateway wraps in satellite relay header
4. Receiving gateway strips relay header
5. Receiving gateway injects compressed payload into local mesh
6. Destination node decompresses with MeshXT

For non-MeshXT (plain text) messages, the gateway compresses them with MeshXT before satellite transmission and marks the relay header accordingly.

## Store & Forward Queue

### Design

```cpp
struct QueueEntry {
    uint32_t    id;           // Unique message ID
    uint8_t     priority;     // 0=emergency, 1=high, 2=normal, 3=low
    uint32_t    timestamp;    // When received from mesh
    uint32_t    ttl;          // Time-to-live (seconds)
    uint8_t     retries;      // Transmission attempts
    uint16_t    payloadLen;
    uint8_t     payload[MAX_SATELLITE_PAYLOAD];
};
```

### Priority Levels

| Priority | Description | TTL | Example |
|----------|-------------|-----|---------|
| 0 - Emergency | Life safety | 24h | SOS, position report |
| 1 - High | Time-sensitive | 12h | Weather alert |
| 2 - Normal | Standard messages | 6h | Text message |
| 3 - Low | Telemetry, status | 2h | Battery level |

### Queue Behaviour

- Maximum queue depth: 64 messages
- FIFO within each priority level
- Higher priority always transmits first
- Expired messages (past TTL) are silently dropped
- Duplicate detection via message ID hash

## Satellite Pass Scheduling

### Algorithm

```
1. Load TLE (Two-Line Element) data for Lacuna constellation
2. Calculate next visible pass using SGP4 propagator
3. Determine pass start time, duration, max elevation
4. Set wake timer for (pass_start - 60 seconds)
5. During pass window:
   a. Drain message queue in priority order
   b. Respect duty cycle limits
   c. Track successful ACKs
6. After pass, update TLE if downlink received
7. Calculate next pass, return to sleep
```

### Simplified Mode (No TLE)

For deployments without accurate TLE data:

```
- Transmit at regular intervals (every 90 minutes)
- 10-minute transmit window
- Less efficient but requires no orbital data
```

### Pass Prediction

- Lacuna operates ~6 satellites in LEO (~550km)
- Average gap between passes: ~90 minutes at mid-latitudes
- Passes last 5–12 minutes depending on elevation
- Higher elevation = stronger signal = higher data rate possible

## Encryption

### Meshtastic Side

- Meshtastic uses AES-128 or AES-256 (channel key)
- MeshXT-Satellite gateway must be a member of the Meshtastic channel
- Gateway decrypts to read routing information, then re-encrypts for mesh injection

### LoRaWAN Side

- LoRaWAN provides its own AES-128 encryption (NwkSKey, AppSKey)
- Payload is encrypted at the LoRaWAN layer automatically
- End-to-end: message is encrypted by Meshtastic AES, then wrapped in LoRaWAN AES

### Double Encryption

The satellite relay payload can optionally remain Meshtastic-encrypted inside the LoRaWAN encryption. This provides:

1. LoRaWAN encryption (network layer) — protects over satellite link
2. Meshtastic encryption (application layer) — protects end-to-end

The gateway only needs to read the Meshtastic header for routing; the message body can stay encrypted if the gateway forwards the raw encrypted payload.

## Duty Cycle Management

### EU868 Regulations

- Sub-band g1 (868.0–868.6 MHz): 1% duty cycle
- At SF12/125kHz: ~1.3s airtime per 51-byte packet
- 1% duty cycle = max 36s per hour = ~27 packets/hour
- With MeshXT compression reducing packet size, effective throughput increases

### Duty Cycle Tracking

```cpp
struct DutyCycleTracker {
    uint32_t windowStart;      // Start of current 1-hour window
    uint32_t airtimeUsedMs;    // Airtime consumed in current window
    uint32_t airtimeLimitMs;   // 36000ms for 1% duty cycle

    bool canTransmit(uint32_t packetAirtimeMs) {
        return (airtimeUsedMs + packetAirtimeMs) <= airtimeLimitMs;
    }
};
```

### Satellite Pass Budget

During a 10-minute pass with 1% duty cycle:
- Available airtime: 6 seconds
- At SF9/125kHz with MeshXT compression: ~8–12 messages per pass
- At SF7/125kHz (if signal allows): ~20–30 messages per pass

## Component Architecture

```
┌─────────────────────────────────────────────┐
│              SatelliteGateway                │
│  ┌──────────────┐  ┌─────────────────────┐  │
│  │  Meshtastic   │  │  LoRaWAN            │  │
│  │  Receiver     │  │  Transmitter        │  │
│  │  (Radio 1)    │  │  (Radio 2)          │  │
│  └──────┬───────┘  └──────────┬──────────┘  │
│         │                      │             │
│         ▼                      ▲             │
│  ┌──────────────────────────────────────┐   │
│  │         PacketTranslator              │   │
│  │  - Meshtastic ↔ LoRaWAN conversion   │   │
│  │  - MeshXT compress/decompress         │   │
│  │  - Header management                 │   │
│  └──────────────────────────────────────┘   │
│                    │                         │
│                    ▼                         │
│  ┌──────────────────────────────────────┐   │
│  │         Message Queue                 │   │
│  │  - Store & forward                    │   │
│  │  - Priority management                │   │
│  │  - TTL enforcement                    │   │
│  └──────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
```

## Future Considerations

- **Multi-satellite support**: Track multiple Lacuna satellites for more frequent passes
- **Adaptive data rate**: Adjust SF based on satellite elevation angle
- **Mesh routing awareness**: Only relay messages tagged for long-range delivery
- **Bidirectional sync**: Full Meshtastic channel sync across satellite link
- **Solar power management**: Coordinate transmit windows with battery state

© Mikoshi Ltd.
