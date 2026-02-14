/**
 * PacketTranslator — Translates between Meshtastic and LoRaWAN satellite framing
 * © Mikoshi Ltd. — Apache 2.0
 */

#ifndef PACKET_TRANSLATOR_H
#define PACKET_TRANSLATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "MeshtasticReceiver.h"

#define RELAY_VERSION          1
#define RELAY_HEADER_SIZE      14   // version(1) + src(4) + dest(4) + channel(1) + timestamp(4)
#define MAX_SATELLITE_PAYLOAD  128

// Priority levels
#define PRIORITY_EMERGENCY  0
#define PRIORITY_HIGH       1
#define PRIORITY_NORMAL     2
#define PRIORITY_LOW        3

struct SatellitePacket {
    uint8_t  version;
    uint32_t sourceNode;
    uint32_t destNode;
    uint8_t  channel;
    uint32_t timestamp;
    uint8_t  payload[MAX_SATELLITE_PAYLOAD - RELAY_HEADER_SIZE];
    uint16_t payloadLen;
    uint8_t  priority;
};

class PacketTranslator {
public:
    PacketTranslator();

    // Meshtastic → Satellite
    bool toSatellite(const MeshtasticPacket &meshPkt, SatellitePacket &satPkt);
    bool serialize(const SatellitePacket &satPkt, uint8_t *out, uint16_t &outLen);

    // Satellite → Meshtastic
    bool fromSatellite(const uint8_t *data, uint16_t len, SatellitePacket &satPkt);
    bool toMeshtastic(const SatellitePacket &satPkt, uint8_t *out, uint16_t &outLen);

private:
    uint8_t determinePriority(const MeshtasticPacket &pkt);
    bool    compressPayload(const uint8_t *in, uint16_t inLen, uint8_t *out, uint16_t &outLen);
    bool    decompressPayload(const uint8_t *in, uint16_t inLen, uint8_t *out, uint16_t &outLen);
};

#endif // PACKET_TRANSLATOR_H
