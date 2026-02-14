/**
 * MeshtasticReceiver — Listens for Meshtastic packets on LoRa Radio 1
 * © Mikoshi Ltd. — Apache 2.0
 */

#include "MeshtasticReceiver.h"
#include "config.h"
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Radio 1: SX1276 for Meshtastic
static SX1276 radio1 = new Module(RADIO1_CS, RADIO1_IRQ, RADIO1_RST);

static volatile bool packetReceived = false;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void onRadio1Receive() {
    packetReceived = true;
}

MeshtasticReceiver::MeshtasticReceiver()
    : _initialized(false), _lastRSSI(0), _lastSNR(0.0f) {}

bool MeshtasticReceiver::begin() {
    if (DEBUG_SERIAL) {
        Serial.println("[MeshtasticRx] Initialising Radio 1 (SX1276)...");
    }

    int state = radio1.begin(
        MESHTASTIC_FREQUENCY,
        MESHTASTIC_BW / 1000.0f,  // kHz
        MESHTASTIC_SF,
        MESHTASTIC_CR,
        MESHTASTIC_SYNC_WORD,
        MESHTASTIC_TX_POWER,
        MESHTASTIC_PREAMBLE
    );

    if (state != RADIOLIB_ERR_NONE) {
        if (DEBUG_SERIAL) {
            Serial.print("[MeshtasticRx] Radio init failed, code: ");
            Serial.println(state);
        }
        return false;
    }

    // Enable CRC
    radio1.setCRC(true);

    // Set up interrupt-driven receive
    radio1.setDio0Action(onRadio1Receive);
    radio1.startReceive();

    _initialized = true;
    if (DEBUG_SERIAL) {
        Serial.println("[MeshtasticRx] Radio 1 ready, listening...");
    }
    return true;
}

bool MeshtasticReceiver::available() {
    return packetReceived;
}

bool MeshtasticReceiver::receive(MeshtasticPacket &packet) {
    if (!packetReceived || !_initialized) return false;
    packetReceived = false;

    uint8_t raw[MESHTASTIC_MAX_PACKET];
    int len = radio1.getPacketLength();
    if (len <= 0 || len > MESHTASTIC_MAX_PACKET) {
        radio1.startReceive();
        return false;
    }

    int state = radio1.readData(raw, len);
    _lastRSSI = radio1.getRSSI();
    _lastSNR  = radio1.getSNR();

    // Restart receive immediately
    radio1.startReceive();

    if (state != RADIOLIB_ERR_NONE) {
        if (DEBUG_SERIAL) {
            Serial.print("[MeshtasticRx] Read error: ");
            Serial.println(state);
        }
        return false;
    }

    if (!parsePacket(raw, len, packet)) {
        return false;
    }

    packet.rssi = _lastRSSI;
    packet.snr  = _lastSNR;

    if (DEBUG_SERIAL) {
        Serial.printf("[MeshtasticRx] Packet from 0x%08X, port=%d, len=%d, RSSI=%d, MeshXT=%s\n",
            packet.source, packet.portnum, packet.payloadLen,
            packet.rssi, packet.isMeshXT ? "yes" : "no");
    }

    return true;
}

bool MeshtasticReceiver::transmit(const uint8_t *data, uint16_t len) {
    if (!_initialized) return false;

    int state = radio1.transmit(data, len);
    // Return to receive mode
    radio1.startReceive();

    return (state == RADIOLIB_ERR_NONE);
}

bool MeshtasticReceiver::parsePacket(const uint8_t *raw, uint16_t rawLen, MeshtasticPacket &packet) {
    // Minimum Meshtastic header: dest(4) + src(4) + id(4) + flags(1) = 13 bytes
    if (rawLen < 13) return false;

    packet.dest   = (raw[0] << 24) | (raw[1] << 16) | (raw[2] << 8) | raw[3];
    packet.source = (raw[4] << 24) | (raw[5] << 16) | (raw[6] << 8) | raw[7];
    packet.id     = (raw[8] << 24) | (raw[9] << 16) | (raw[10] << 8) | raw[11];
    packet.flags  = raw[12];

    // Payload starts after header
    uint16_t payloadStart = 13;

    // Port number is in the first byte of the payload (simplified)
    if (rawLen > payloadStart) {
        packet.portnum = raw[payloadStart];
        payloadStart += 1;
    } else {
        packet.portnum = 0;
    }

    packet.payloadLen = rawLen - payloadStart;
    if (packet.payloadLen > sizeof(packet.payload)) {
        packet.payloadLen = sizeof(packet.payload);
    }
    memcpy(packet.payload, raw + payloadStart, packet.payloadLen);

    // Detect MeshXT packets
    packet.isMeshXT = (packet.portnum == PORTNUM_PRIVATE_APP) &&
                       isMeshXTPacket(packet.payload, packet.payloadLen);

    return true;
}

bool MeshtasticReceiver::isMeshXTPacket(const uint8_t *payload, uint16_t len) {
    // MeshXT header starts with magic bytes 0x4D 0x58 ("MX")
    if (len < 2) return false;
    return (payload[0] == 0x4D && payload[1] == 0x58);
}
