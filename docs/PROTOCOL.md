# Satellite Relay Protocol

## Packet Format

Messages sent via satellite use a compact relay header + MeshXT compressed payload.

### Relay Header (6 bytes)

```
Byte 0:    [VVVV TTTT]  Version (4 bits) | Type (4 bits)
Byte 1-2:  Sender node ID (16-bit, from Meshtastic)
Byte 3:    Channel index
Byte 4-5:  Timestamp (minutes since midnight UTC, 16-bit)
Byte 6+:   MeshXT compressed payload
```

### Message Types

| Type | Value | Description |
|------|-------|-------------|
| TEXT | 0x01 | Text message |
| SOS  | 0x02 | Emergency/SOS (priority) |
| POS  | 0x03 | Position report |
| ACK  | 0x04 | Acknowledgement |
| PING | 0x05 | Connectivity check |

### Total Packet Size

```
LoRaWAN overhead:     13 bytes (header + MIC)
Relay header:          6 bytes
MeshXT payload:       10-50 bytes (compressed text + FEC)
─────────────────────────────────
Total:                29-69 bytes
```

Fits within LoRaWAN DR0 (51 byte payload) for short messages, DR1+ for longer messages.

## Encryption

- **Meshtastic side:** AES-128 or AES-256 (Meshtastic channel encryption)
- **Satellite side:** LoRaWAN AES-128 (network + application layer)
- Gateway decrypts Meshtastic → re-encrypts as LoRaWAN
- End-to-end encryption option: encrypt at source, gateway passes opaque blob

## Flow

### Uplink (Mesh → Satellite)
1. Meshtastic node sends message on mesh
2. Gateway receives via LoRa radio 1
3. If MeshXT packet: decompress to get original text
4. If plain text: use as-is
5. Create relay header with sender ID, timestamp, channel
6. Compress text with MeshXT (if not already)
7. Wrap in LoRaWAN frame
8. Queue for next satellite pass
9. Transmit during pass

### Downlink (Satellite → Mesh)
1. LoRaWAN downlink received during satellite pass
2. Strip LoRaWAN frame
3. Parse relay header
4. Decompress MeshXT payload
5. Create Meshtastic packet with original sender info
6. Inject into local mesh via LoRa radio 1
