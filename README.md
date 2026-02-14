# MeshXT-Satellite — Meshtastic to Satellite Bridge

> Send Meshtastic messages via satellite. No internet, no cell towers, no line of sight needed.

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

## The Problem

- Meshtastic is limited by radio line of sight — mountains, distance, and terrain block signals
- LoRa satellite (Lacuna Space) can relay globally but expects LoRaWAN, not Meshtastic protocol
- No bridge exists between the two

## The Solution

MeshXT-Satellite is a gateway device that translates between Meshtastic mesh and LoRaWAN satellite. MeshXT compression ensures messages are small enough for satellite uplink constraints. It runs on dual-radio hardware — one radio for Meshtastic mesh, one for LoRaWAN satellite uplink.

## Architecture

```
[Meshtastic Node] --LoRa--> [MeshXT-Satellite Gateway] --LoRaWAN--> [Lacuna Satellite]
                                                                          |
                                                                      [Internet]
                                                                          |
[Meshtastic Node] <--LoRa-- [MeshXT-Satellite Gateway] <--LoRaWAN-- [Ground Station]
```

## How It Works

1. Your Meshtastic nodes communicate as normal on the local mesh
2. The MeshXT-Satellite gateway listens for mesh traffic
3. When a message needs to go beyond radio range, the gateway compresses it with MeshXT and sends it as a LoRaWAN packet to the next Lacuna satellite passing overhead
4. Lacuna's ground station receives it and forwards via internet
5. On the receiving end, another MeshXT-Satellite gateway picks it up and injects it into the local Meshtastic mesh

## Satellite Timing

- Lacuna satellites orbit at ~550km altitude
- Each satellite passes overhead every ~90 minutes
- Pass duration: ~10 minutes
- Messages are queued and sent during the next pass (store & forward)
- With MeshXT compression, more messages fit in each pass window

## Hardware Requirements

Recommended dual-radio boards:

- **LilyGO T-Beam + LoRaWAN HAT** — Meshtastic on SX1276, LoRaWAN on second radio
- **Heltec CubeCell + Heltec V3** — paired, one for each protocol
- **RAK WisBlock dual-LoRa** — RAK4631 with two LoRa modules
- **Raspberry Pi + 2x LoRa HATs** — most flexible, runs gateway as Linux service
- **Single radio with time-division** — switch between Meshtastic and LoRaWAN (simpler hardware, more complex software)

See [docs/HARDWARE.md](docs/HARDWARE.md) for detailed hardware guidance.

## Lacuna Space Integration

- Free developer access via The Things Industries partnership
- LS300 dev kit for evaluation
- [https://lacuna-space.com](https://lacuna-space.com)

See [docs/LACUNA.md](docs/LACUNA.md) for integration details.

## Why MeshXT Matters for Satellite

- Satellite LoRa has stricter packet size limits than ground LoRa
- MeshXT compression reduces message size by 15–50%
- Reed-Solomon FEC is critical for satellite links (higher error rate)
- Smaller packets = less airtime = more messages per satellite pass
- A compressed "SOS" takes 1 byte vs 3 bytes uncompressed

## Quick Start

```bash
# Clone
git clone https://github.com/DarrenEdwards111/MeshXT-Satellite.git
cd MeshXT-Satellite

# Install PlatformIO
pip install platformio

# Configure (edit src/gateway/config.h with your keys)
cp src/gateway/config.h src/gateway/config_local.h

# Build
pio run

# Flash
pio run --target upload
```

## Project Status

- 🟡 In Development
- Gateway architecture designed
- MeshXT compression ready
- Awaiting Lacuna dev kit for testing

## Related Projects

- [MeshXT](https://github.com/DarrenEdwards111/MeshXT) — compression + FEC library (npm)
- [Meshtastic-MeshXT-Firmware](https://github.com/DarrenEdwards111/Meshtastic-MeshXT-Firmware) — combined Meshtastic + MeshXT firmware
- [Mikoshi](https://mikoshi.co.uk) — parent platform

## Get in Touch

- 🐛 Bugs → [GitHub Issues](https://github.com/DarrenEdwards111/MeshXT-Satellite/issues)
- 💡 Ideas → [GitHub Discussions](https://github.com/DarrenEdwards111/MeshXT-Satellite/discussions)
- 📧 Contact → mikoshiuk@gmail.com

## License

Apache 2.0 — see [LICENSE](LICENSE).

© Mikoshi Ltd.
