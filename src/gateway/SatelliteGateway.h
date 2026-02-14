/**
 * SatelliteGateway — Main gateway bridging Meshtastic ↔ LoRaWAN satellite
 * © Mikoshi Ltd. — Apache 2.0
 */

#ifndef SATELLITE_GATEWAY_H
#define SATELLITE_GATEWAY_H

#include "MeshtasticReceiver.h"
#include "LoRaWANTransmitter.h"
#include "PacketTranslator.h"
#include "config.h"

struct QueueEntry {
    uint32_t id;
    uint8_t  priority;
    uint32_t timestamp;
    uint32_t ttl;
    uint8_t  retries;
    uint16_t payloadLen;
    uint8_t  payload[MAX_SATELLITE_PAYLOAD];
};

class SatelliteGateway {
public:
    SatelliteGateway();

    void setup();
    void loop();

private:
    MeshtasticReceiver  _meshRx;
    LoRaWANTransmitter  _loraWAN;
    PacketTranslator    _translator;

    QueueEntry _queue[QUEUE_MAX_ENTRIES];
    uint8_t    _queueCount;

    uint32_t _lastPassTime;
    uint32_t _nextPassTime;
    bool     _inPassWindow;

    // Core operations
    void handleMeshPacket();
    void handleSatellitePass();
    void handleDownlink();
    void updatePassSchedule();

    // Queue management
    bool enqueue(const SatellitePacket &pkt);
    bool dequeueHighestPriority(QueueEntry &entry);
    void purgeExpired();
    uint32_t ttlForPriority(uint8_t priority);

    // Status
    void printStatus();
};

#endif // SATELLITE_GATEWAY_H
