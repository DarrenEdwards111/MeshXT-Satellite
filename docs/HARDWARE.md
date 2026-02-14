# Hardware Guide

## Recommended Configurations

### Option 1: Dual Heltec (Simplest)
- **Heltec V3** — runs Meshtastic + MeshXT firmware, handles mesh
- **Heltec CubeCell HTCC-AB02** — dedicated LoRaWAN uplink to Lacuna
- Connect via UART between boards
- Cost: ~£40-60 total

### Option 2: RAK WisBlock (Most Integrated)
- **RAK4631** (nRF52840 + SX1262) — main processor
- **RAK13300** (SX1262 LoRaWAN module) — second radio for satellite
- Single board, dual radio
- Cost: ~£50-70

### Option 3: Raspberry Pi (Most Flexible)
- **Raspberry Pi 4/5** — runs gateway as Linux service
- **2x LoRa HATs** (e.g., RAK2245, Waveshare SX1262)
- Can run MeshXT Node.js library directly
- Most flexible, easiest to develop on
- Cost: ~£80-120

### Option 4: LilyGO T-Beam
- **T-Beam v1.2** — ESP32 + SX1276 + GPS
- Add second LoRa module via SPI
- GPS useful for satellite pass prediction
- Cost: ~£30-50

### Option 5: Single Radio Time-Division
- Any Meshtastic device (Heltec V3, T-Beam, etc.)
- Alternates between Meshtastic mesh mode and LoRaWAN satellite mode
- Simpler hardware, more complex software
- May miss mesh packets during satellite transmit windows

## Antennas

### Ground (Meshtastic Mesh)
- Standard 868MHz/915MHz antenna
- Omni-directional whip or collinear

### Satellite Uplink
- **Upward-facing patch antenna** recommended for satellite
- Higher gain toward zenith
- 868MHz (EU) or 915MHz (US)
- Circular polarisation preferred (reduces Faraday rotation losses)

## Power
- 5V USB or LiPo battery
- Solar panel recommended for remote deployment
- Typical consumption: ~200mA active, ~20mA sleep
