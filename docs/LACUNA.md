# Lacuna Space Integration

## Overview

Lacuna Space operates a constellation of LEO satellites with LoRa receivers. Their satellites orbit at ~550km and pass overhead every ~90 minutes, each pass lasting ~10 minutes.

## Getting Started

### 1. Sign Up
- Free developer access via The Things Industries partnership
- https://lacuna-space.com

### 2. Get Hardware
- **Lacuna LS300** — evaluation sensor & relay device (~£50-100)
- Or use any LoRaWAN device with satellite-compatible antenna

### 3. Register Device
- Register your device on The Things Network (TTN) or The Things Industries
- Lacuna's satellites act as LoRaWAN gateways in the sky
- Standard LoRaWAN OTAA join procedure

### 4. Configure MeshXT-Satellite
Edit `src/gateway/config.h` with your:
- DevEUI (device identifier)
- AppEUI (application identifier)  
- AppKey (encryption key)

## Satellite Pass Scheduling

Lacuna satellites are in LEO polar orbits:
- **Altitude:** ~550km
- **Inclination:** ~97° (sun-synchronous)
- **Period:** ~96 minutes
- **Pass duration:** 5-12 minutes (depending on elevation)
- **Passes per day:** 4-8 visible passes per location

The gateway queues messages and transmits during satellite passes. Higher elevation passes (satellite more directly overhead) have better link budgets.

## Packet Constraints

| Parameter | Value |
|-----------|-------|
| Max payload | 51 bytes (DR0) to 222 bytes (DR5) |
| Duty cycle | 1% (EU868) |
| Tx power | 14 dBm (EU868) |
| Spreading factor | SF12 (best for satellite) |

With MeshXT compression, a typical 40-byte text message becomes ~20-25 bytes — well within satellite constraints even at the lowest data rate.

## Fair Use Policy

Lacuna's free developer tier has a fair use policy:
- Reasonable message volume for evaluation
- Not for production traffic without commercial agreement
- Contact Lacuna for commercial use
