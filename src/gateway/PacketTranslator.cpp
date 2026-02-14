/**
 * PacketTranslator — Translates between Meshtastic and LoRaWAN satellite framing
 * © Mikoshi Ltd. — Apache 2.0
 */

#include "PacketTranslator.h"
#include "config.h"
#include "../meshxt/MeshXTCompress.h"
#include "../meshxt/MeshXTFEC.h"
#include <Arduino.h>
#include <string.h>

PacketTranslator::PacketTranslator() {}

bool PacketTranslator::toSatellite(const MeshtasticPacket &meshPkt, SatellitePacket &satPkt) {
    satPkt.version    = RELAY_VERSION;
    satPkt.sourceNode = meshPkt.source;
    satPkt.destNode   = meshPkt.dest;
    satPkt.channel    = 0;  // Default channel; could extract from Meshtastic header
    satPkt.timestamp  = (uint32_t)(millis() / 1000);  // Relative timestamp
    satPkt.priority   = determinePriority(meshPkt);

    if (meshPkt.isMeshXT) {
        // Already compressed — pass through the MeshXT payload directly
        if (meshPkt.payloadLen > sizeof(satPkt.payload)) return false;
        memcpy(satPkt.payload, meshPkt.payload, meshPkt.payloadLen);
        satPkt.payloadLen = meshPkt.payloadLen;

        if (DEBUG_SERIAL) {
            Serial.printf("[Translator] MeshXT passthrough: %d bytes\n", satPkt.payloadLen);
        }
    } else {
        // Plain text — compress with MeshXT for satellite
        if (!compressPayload(meshPkt.payload, meshPkt.payloadLen,
                             satPkt.payload, satPkt.payloadLen)) {
            // Compression failed — send raw (may be too large)
            if (meshPkt.payloadLen > sizeof(satPkt.payload)) return false;
            memcpy(satPkt.payload, meshPkt.payload, meshPkt.payloadLen);
            satPkt.payloadLen = meshPkt.payloadLen;

            if (DEBUG_SERIAL) {
                Serial.println("[Translator] Compression failed, sending raw.");
            }
        } else {
            if (DEBUG_SERIAL) {
                Serial.printf("[Translator] Compressed %d -> %d bytes (%.0f%% reduction)\n",
                    meshPkt.payloadLen, satPkt.payloadLen,
                    (1.0f - (float)satPkt.payloadLen / meshPkt.payloadLen) * 100.0f);
            }
        }
    }

    return true;
}

bool PacketTranslator::serialize(const SatellitePacket &satPkt, uint8_t *out, uint16_t &outLen) {
    uint16_t totalLen = RELAY_HEADER_SIZE + satPkt.payloadLen;
    if (totalLen > MAX_SATELLITE_PAYLOAD) return false;

    uint16_t idx = 0;

    // Version
    out[idx++] = satPkt.version;

    // Source node (big-endian)
    out[idx++] = (satPkt.sourceNode >> 24) & 0xFF;
    out[idx++] = (satPkt.sourceNode >> 16) & 0xFF;
    out[idx++] = (satPkt.sourceNode >> 8)  & 0xFF;
    out[idx++] = satPkt.sourceNode & 0xFF;

    // Dest node (big-endian)
    out[idx++] = (satPkt.destNode >> 24) & 0xFF;
    out[idx++] = (satPkt.destNode >> 16) & 0xFF;
    out[idx++] = (satPkt.destNode >> 8)  & 0xFF;
    out[idx++] = satPkt.destNode & 0xFF;

    // Channel
    out[idx++] = satPkt.channel;

    // Timestamp (big-endian)
    out[idx++] = (satPkt.timestamp >> 24) & 0xFF;
    out[idx++] = (satPkt.timestamp >> 16) & 0xFF;
    out[idx++] = (satPkt.timestamp >> 8)  & 0xFF;
    out[idx++] = satPkt.timestamp & 0xFF;

    // Payload
    memcpy(out + idx, satPkt.payload, satPkt.payloadLen);
    idx += satPkt.payloadLen;

    outLen = idx;
    return true;
}

bool PacketTranslator::fromSatellite(const uint8_t *data, uint16_t len, SatellitePacket &satPkt) {
    if (len < RELAY_HEADER_SIZE) return false;

    uint16_t idx = 0;

    satPkt.version = data[idx++];
    if (satPkt.version != RELAY_VERSION) {
        if (DEBUG_SERIAL) {
            Serial.printf("[Translator] Unknown relay version: %d\n", satPkt.version);
        }
        return false;
    }

    satPkt.sourceNode = ((uint32_t)data[idx] << 24) | ((uint32_t)data[idx+1] << 16) |
                        ((uint32_t)data[idx+2] << 8) | data[idx+3];
    idx += 4;

    satPkt.destNode = ((uint32_t)data[idx] << 24) | ((uint32_t)data[idx+1] << 16) |
                      ((uint32_t)data[idx+2] << 8) | data[idx+3];
    idx += 4;

    satPkt.channel = data[idx++];

    satPkt.timestamp = ((uint32_t)data[idx] << 24) | ((uint32_t)data[idx+1] << 16) |
                       ((uint32_t)data[idx+2] << 8) | data[idx+3];
    idx += 4;

    satPkt.payloadLen = len - idx;
    if (satPkt.payloadLen > sizeof(satPkt.payload)) return false;
    memcpy(satPkt.payload, data + idx, satPkt.payloadLen);

    return true;
}

bool PacketTranslator::toMeshtastic(const SatellitePacket &satPkt, uint8_t *out, uint16_t &outLen) {
    // Reconstruct a Meshtastic-compatible packet for mesh injection
    uint16_t idx = 0;

    // Dest (big-endian)
    out[idx++] = (satPkt.destNode >> 24) & 0xFF;
    out[idx++] = (satPkt.destNode >> 16) & 0xFF;
    out[idx++] = (satPkt.destNode >> 8)  & 0xFF;
    out[idx++] = satPkt.destNode & 0xFF;

    // Source (big-endian)
    out[idx++] = (satPkt.sourceNode >> 24) & 0xFF;
    out[idx++] = (satPkt.sourceNode >> 16) & 0xFF;
    out[idx++] = (satPkt.sourceNode >> 8)  & 0xFF;
    out[idx++] = satPkt.sourceNode & 0xFF;

    // Packet ID (generate new one)
    uint32_t newId = micros();
    out[idx++] = (newId >> 24) & 0xFF;
    out[idx++] = (newId >> 16) & 0xFF;
    out[idx++] = (newId >> 8)  & 0xFF;
    out[idx++] = newId & 0xFF;

    // Flags
    out[idx++] = 0x00;

    // Port number — check if MeshXT
    bool isMeshXT = (satPkt.payloadLen >= 2 &&
                     satPkt.payload[0] == 0x4D && satPkt.payload[1] == 0x58);
    out[idx++] = isMeshXT ? PORTNUM_PRIVATE_APP : PORTNUM_TEXT_MESSAGE_APP;

    // Payload
    memcpy(out + idx, satPkt.payload, satPkt.payloadLen);
    idx += satPkt.payloadLen;

    outLen = idx;
    return true;
}

uint8_t PacketTranslator::determinePriority(const MeshtasticPacket &pkt) {
    // Check for emergency/SOS keywords in plain text messages
    if (!pkt.isMeshXT && pkt.payloadLen > 0) {
        // Simple SOS detection
        if (pkt.payloadLen >= 3) {
            if ((pkt.payload[0] == 'S' || pkt.payload[0] == 's') &&
                (pkt.payload[1] == 'O' || pkt.payload[1] == 'o') &&
                (pkt.payload[2] == 'S' || pkt.payload[2] == 's')) {
                return PRIORITY_EMERGENCY;
            }
        }
    }

    // Position reports are higher priority
    if (pkt.portnum == PORTNUM_POSITION_APP) {
        return PRIORITY_HIGH;
    }

    return PRIORITY_NORMAL;
}

bool PacketTranslator::compressPayload(const uint8_t *in, uint16_t inLen,
                                        uint8_t *out, uint16_t &outLen) {
#if MESHXT_COMPRESSION_ENABLED
    MeshXTCompress compressor;
    int result = compressor.compress(in, inLen, out, outLen);
    if (result < 0) return false;

#if MESHXT_FEC_ENABLED
    // Add FEC parity
    uint8_t fecBuf[MAX_SATELLITE_PAYLOAD];
    uint16_t fecLen;
    MeshXTFEC fec;
    if (fec.encode(out, outLen, fecBuf, fecLen, MESHXT_FEC_REDUNDANCY)) {
        memcpy(out, fecBuf, fecLen);
        outLen = fecLen;
    }
#endif

    return true;
#else
    // No compression — just copy
    memcpy(out, in, inLen);
    outLen = inLen;
    return true;
#endif
}

bool PacketTranslator::decompressPayload(const uint8_t *in, uint16_t inLen,
                                          uint8_t *out, uint16_t &outLen) {
#if MESHXT_COMPRESSION_ENABLED
    const uint8_t *dataIn = in;
    uint16_t dataLen = inLen;

#if MESHXT_FEC_ENABLED
    // Strip and verify FEC
    uint8_t corrected[MAX_SATELLITE_PAYLOAD];
    uint16_t correctedLen;
    MeshXTFEC fec;
    if (fec.decode(in, inLen, corrected, correctedLen)) {
        dataIn = corrected;
        dataLen = correctedLen;
    }
    // If FEC decode fails, try decompressing raw data anyway
#endif

    MeshXTCompress compressor;
    int result = compressor.decompress(dataIn, dataLen, out, outLen);
    return (result >= 0);
#else
    memcpy(out, in, inLen);
    outLen = inLen;
    return true;
#endif
}
