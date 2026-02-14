/**
 * LoRaWANTransmitter — Sends compressed packets via LoRaWAN to Lacuna satellites
 * © Mikoshi Ltd. — Apache 2.0
 */

#ifndef LORAWAN_TRANSMITTER_H
#define LORAWAN_TRANSMITTER_H

#include <stdint.h>
#include <stdbool.h>

#define LORAWAN_MAX_PAYLOAD  128
#define DOWNLINK_BUFFER_SIZE 256

struct DownlinkMessage {
    uint8_t  payload[DOWNLINK_BUFFER_SIZE];
    uint16_t len;
    uint8_t  fport;
    bool     pending;
};

class LoRaWANTransmitter {
public:
    LoRaWANTransmitter();

    bool begin();
    bool join();
    bool isJoined() const { return _joined; }

    bool send(const uint8_t *payload, uint16_t len, uint8_t fport);
    bool canTransmit(uint32_t packetAirtimeMs);

    bool hasDownlink() const { return _downlink.pending; }
    DownlinkMessage getDownlink();

    uint32_t getAirtimeUsedMs() const { return _airtimeUsedMs; }
    uint32_t getAirtimeRemainingMs() const;

private:
    bool     _initialized;
    bool     _joined;
    uint32_t _airtimeUsedMs;
    uint32_t _dutyCycleWindowStart;
    DownlinkMessage _downlink;

    void     resetDutyCycleIfNeeded();
    uint32_t estimateAirtimeMs(uint16_t payloadLen);
};

#endif // LORAWAN_TRANSMITTER_H
