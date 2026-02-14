/**
 * LoRaWANTransmitter — Sends compressed packets via LoRaWAN to Lacuna satellites
 * © Mikoshi Ltd. — Apache 2.0
 */

#include "LoRaWANTransmitter.h"
#include "config.h"
#include <Arduino.h>
#include <RadioLib.h>

// Radio 2: SX1262 for LoRaWAN
static SX1262 radio2 = new Module(RADIO2_CS, RADIO2_IRQ, RADIO2_RST, RADIO2_BUSY);
static LoRaWANNode node(&radio2, &EU868);

// LoRaWAN credentials
static const uint8_t devEui[]  = LORAWAN_DEVEUI;
static const uint8_t appEui[]  = LORAWAN_APPEUI;
static const uint8_t appKey[]  = LORAWAN_APPKEY;

LoRaWANTransmitter::LoRaWANTransmitter()
    : _initialized(false)
    , _joined(false)
    , _airtimeUsedMs(0)
    , _dutyCycleWindowStart(0) {
    _downlink.pending = false;
}

bool LoRaWANTransmitter::begin() {
    if (DEBUG_SERIAL) {
        Serial.println("[LoRaWAN] Initialising Radio 2 (SX1262)...");
    }

    int state = radio2.begin();
    if (state != RADIOLIB_ERR_NONE) {
        if (DEBUG_SERIAL) {
            Serial.print("[LoRaWAN] Radio init failed, code: ");
            Serial.println(state);
        }
        return false;
    }

    _initialized = true;
    _dutyCycleWindowStart = millis();

    if (DEBUG_SERIAL) {
        Serial.println("[LoRaWAN] Radio 2 ready.");
    }
    return true;
}

bool LoRaWANTransmitter::join() {
    if (!_initialized) return false;

    if (DEBUG_SERIAL) {
        Serial.println("[LoRaWAN] Attempting OTAA join...");
    }

#if LORAWAN_USE_OTAA
    int state = node.beginOTAA(appEui, appKey, devEui);
#else
    uint8_t nwkSKey[] = LORAWAN_NWKSKEY;
    uint8_t appSKey[] = LORAWAN_APPSKEY;
    int state = node.beginABP(LORAWAN_DEVADDR, nwkSKey, appSKey);
#endif

    if (state != RADIOLIB_ERR_NONE) {
        if (DEBUG_SERIAL) {
            Serial.print("[LoRaWAN] Join failed, code: ");
            Serial.println(state);
        }
        return false;
    }

    _joined = true;
    if (DEBUG_SERIAL) {
        Serial.println("[LoRaWAN] Joined successfully.");
    }
    return true;
}

bool LoRaWANTransmitter::send(const uint8_t *payload, uint16_t len, uint8_t fport) {
    if (!_initialized || !_joined) return false;
    if (len > LORAWAN_MAX_PAYLOAD) return false;

    resetDutyCycleIfNeeded();

    uint32_t airtime = estimateAirtimeMs(len);
    if (!canTransmit(airtime)) {
        if (DEBUG_SERIAL) {
            Serial.println("[LoRaWAN] Duty cycle limit reached, deferring.");
        }
        return false;
    }

    if (DEBUG_SERIAL) {
        Serial.printf("[LoRaWAN] Sending %d bytes on fport %d...\n", len, fport);
    }

    // Prepare downlink buffer
    uint8_t downBuf[DOWNLINK_BUFFER_SIZE];
    size_t  downLen = 0;

    int state = node.sendReceive(payload, len, fport, downBuf, &downLen);

    if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_LORAWAN_NO_DOWNLINK) {
        _airtimeUsedMs += airtime;

        if (DEBUG_SERIAL) {
            Serial.printf("[LoRaWAN] Sent OK. Airtime: %dms, total used: %dms/%dms\n",
                airtime, _airtimeUsedMs, DUTY_CYCLE_LIMIT_MS);
        }

        // Check for downlink
        if (state == RADIOLIB_ERR_NONE && downLen > 0) {
            memcpy(_downlink.payload, downBuf, downLen);
            _downlink.len = downLen;
            _downlink.fport = fport;
            _downlink.pending = true;

            if (DEBUG_SERIAL) {
                Serial.printf("[LoRaWAN] Downlink received: %d bytes\n", downLen);
            }
        }

        return true;
    }

    if (DEBUG_SERIAL) {
        Serial.print("[LoRaWAN] Send failed, code: ");
        Serial.println(state);
    }
    return false;
}

bool LoRaWANTransmitter::canTransmit(uint32_t packetAirtimeMs) {
    resetDutyCycleIfNeeded();
    return (_airtimeUsedMs + packetAirtimeMs) <= DUTY_CYCLE_LIMIT_MS;
}

DownlinkMessage LoRaWANTransmitter::getDownlink() {
    DownlinkMessage msg = _downlink;
    _downlink.pending = false;
    return msg;
}

uint32_t LoRaWANTransmitter::getAirtimeRemainingMs() const {
    if (_airtimeUsedMs >= DUTY_CYCLE_LIMIT_MS) return 0;
    return DUTY_CYCLE_LIMIT_MS - _airtimeUsedMs;
}

void LoRaWANTransmitter::resetDutyCycleIfNeeded() {
    uint32_t now = millis();
    if ((now - _dutyCycleWindowStart) >= DUTY_CYCLE_WINDOW_MS) {
        _dutyCycleWindowStart = now;
        _airtimeUsedMs = 0;
        if (DEBUG_SERIAL) {
            Serial.println("[LoRaWAN] Duty cycle window reset.");
        }
    }
}

uint32_t LoRaWANTransmitter::estimateAirtimeMs(uint16_t payloadLen) {
    // Simplified LoRa airtime estimation for SF9/125kHz
    // Real calculation depends on SF, BW, CR, preamble, header mode
    // This is a conservative estimate
    float symbolTime = (1 << LORAWAN_SF) / 125.0f;  // ms per symbol
    float preambleTime = (8 + 4.25f) * symbolTime;
    float payloadSymbols = 8 + ((8 * payloadLen - 4 * LORAWAN_SF + 28 + 16) /
                                (4 * (LORAWAN_SF - 2))) * 5;
    if (payloadSymbols < 8) payloadSymbols = 8;
    float payloadTime = payloadSymbols * symbolTime;
    return (uint32_t)(preambleTime + payloadTime + 0.5f);
}
