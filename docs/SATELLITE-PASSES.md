# Satellite Pass Scheduling

## How It Works

Lacuna's satellites are in Low Earth Orbit (LEO). They're not geostationary — they move across the sky. Your gateway needs to know when a satellite is overhead to transmit.

## Pass Prediction

Satellites follow predictable orbits described by TLE (Two-Line Element) data. The gateway can predict passes using:

1. **TLE data** — updated periodically from CelesTrak or Space-Track
2. **SGP4 algorithm** — standard orbital propagator
3. **Local position** — your lat/lon/altitude

## Pass Characteristics

| Parameter | Typical Value |
|-----------|---------------|
| Passes per day | 4-8 (depends on latitude) |
| Pass duration | 5-12 minutes |
| Best elevation | >30° above horizon |
| Worst elevation | <10° (long path, high attenuation) |
| Time between passes | 90-120 minutes |

## Store & Forward Strategy

1. Messages arrive from the Meshtastic mesh at any time
2. Gateway compresses with MeshXT and adds to queue
3. Queue persists in flash memory (survives power loss)
4. When satellite pass begins, gateway transmits queued messages
5. Priority: SOS/emergency first, then oldest messages
6. After pass ends, remaining messages wait for next pass

## Duty Cycle Management

EU868 regulations limit transmit duty cycle to 1%:
- In a 10-minute pass window: ~6 seconds of transmit time
- At SF12/125kHz: ~1.5 seconds per 20-byte packet
- That's ~4 messages per pass
- With MeshXT compression: effectively ~6-8 original messages per pass

## Latitude Effects

- **Polar regions** (>60°): More passes per day (8+), satellites converge
- **Mid-latitudes** (30-60°): 4-6 passes per day — UK is here
- **Equatorial** (<30°): Fewer passes (3-4), but longer duration
