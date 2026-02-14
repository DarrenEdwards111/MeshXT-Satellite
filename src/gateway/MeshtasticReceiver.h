/**
 * MeshtasticReceiver — Listens for Meshtastic packets on LoRa Radio 1
 * © Mikoshi Ltd. — Apache 2.0
 */

#ifndef MESHTASTIC_RECEIVER_H
#define MESHTASTIC_RECEIVER_H

#include <stdint.h>
#include <stdbool.h>

// Meshtastic port numbers
#define PORTNUM_TEXT_MESSAGE_APP   1
#define PORTNUM_POSITION_APP      3
#define PORTNUM_NODEINFO_APP      4
#define PORTNUM_PRIVATE_APP       256

// Maximum raw packet size
#define MESHTASTIC_MAX_PACKET     256

struct MeshtasticPacket {
    uint32_t dest;
    uint32_t source;
    uint32_t id;
    uint8_t  flags;
    uint8_t  portnum;
    bool     isMeshXT;        // True if PRIVATE_APP with MeshXT header
    uint8_t  payload[237];
    uint16_t payloadLen;
    int16_t  rssi;
    float    snr;
};

class MeshtasticReceiver {
public:
    MeshtasticReceiver();

    bool begin();
    bool available();
    bool receive(MeshtasticPacket &packet);
    bool transmit(const uint8_t *data, uint16_t len);

    int16_t lastRSSI() const { return _lastRSSI; }
    float   lastSNR()  const { return _lastSNR; }

private:
    bool    _initialized;
    int16_t _lastRSSI;
    float   _lastSNR;

    bool parsePacket(const uint8_t *raw, uint16_t rawLen, MeshtasticPacket &packet);
    bool isMeshXTPacket(const uint8_t *payload, uint16_t len);
};

#endif // MESHTASTIC_RECEIVER_H
