/**
 * SatelliteGateway — Main gateway bridging Meshtastic ↔ LoRaWAN satellite
 * © Mikoshi Ltd. — Apache 2.0
 */

#include "SatelliteGateway.h"
#include <Arduino.h>

SatelliteGateway::SatelliteGateway()
    : _queueCount(0)
    , _lastPassTime(0)
    , _nextPassTime(0)
    , _inPassWindow(false) {}

void SatelliteGateway::setup() {
    Serial.begin(DEBUG_BAUD);
    delay(2000);

    Serial.println("========================================");
    Serial.println("  MeshXT-Satellite Gateway v0.1.0");
    Serial.println("  Meshtastic <-> LoRaWAN Satellite");
    Serial.println("  (c) Mikoshi Ltd.");
    Serial.println("========================================");

    // Initialise status LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialise Meshtastic radio (Radio 1)
    Serial.println("\n[Gateway] Starting Meshtastic receiver...");
    if (!_meshRx.begin()) {
        Serial.println("[Gateway] FATAL: Meshtastic radio failed!");
        while (true) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(200);
        }
    }

    // Initialise LoRaWAN radio (Radio 2)
    Serial.println("[Gateway] Starting LoRaWAN transmitter...");
    if (!_loraWAN.begin()) {
        Serial.println("[Gateway] FATAL: LoRaWAN radio failed!");
        while (true) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(500);
        }
    }

    // Join LoRaWAN network
    Serial.println("[Gateway] Joining LoRaWAN network...");
    int joinAttempts = 0;
    while (!_loraWAN.join() && joinAttempts < 5) {
        joinAttempts++;
        Serial.printf("[Gateway] Join attempt %d/5 failed, retrying...\n", joinAttempts);
        delay(10000);
    }

    if (!_loraWAN.isJoined()) {
        Serial.println("[Gateway] WARNING: LoRaWAN join failed. Will retry during pass.");
    } else {
        Serial.println("[Gateway] LoRaWAN joined successfully.");
    }

    // Schedule first satellite pass
    _nextPassTime = millis() + SAT_PASS_INTERVAL_MS;
    Serial.printf("[Gateway] Next satellite pass in %d minutes.\n",
                  SAT_PASS_INTERVAL_MS / 60000);

    Serial.println("\n[Gateway] Ready. Listening for Meshtastic traffic...\n");
    digitalWrite(LED_PIN, HIGH);
}

void SatelliteGateway::loop() {
    // 1. Check for incoming Meshtastic packets
    if (_meshRx.available()) {
        handleMeshPacket();
    }

    // 2. Check if we're in a satellite pass window
    uint32_t now = millis();
    if (!_inPassWindow && now >= (_nextPassTime - SAT_PASS_WAKE_EARLY_MS)) {
        _inPassWindow = true;
        Serial.println("\n[Gateway] === SATELLITE PASS WINDOW OPEN ===");
        Serial.printf("[Gateway] Queue: %d messages\n", _queueCount);
    }

    if (_inPassWindow) {
        handleSatellitePass();

        // Check if pass window has ended
        if (now >= (_nextPassTime + SAT_PASS_DURATION_MS)) {
            _inPassWindow = false;
            _lastPassTime = _nextPassTime;
            _nextPassTime = now + SAT_PASS_INTERVAL_MS;
            Serial.println("[Gateway] === SATELLITE PASS WINDOW CLOSED ===");
            Serial.printf("[Gateway] Next pass in %d minutes. Queue: %d remaining.\n",
                          SAT_PASS_INTERVAL_MS / 60000, _queueCount);
        }
    }

    // 3. Check for LoRaWAN downlink
    if (_loraWAN.hasDownlink()) {
        handleDownlink();
    }

    // 4. Periodic maintenance
    static uint32_t lastMaintenance = 0;
    if (now - lastMaintenance > 60000) {  // Every minute
        purgeExpired();
        lastMaintenance = now;
    }

    // 5. Status report every 5 minutes
    static uint32_t lastStatus = 0;
    if (now - lastStatus > 300000) {
        printStatus();
        lastStatus = now;
    }
}

void SatelliteGateway::handleMeshPacket() {
    MeshtasticPacket meshPkt;
    if (!_meshRx.receive(meshPkt)) return;

    // Filter: only relay text messages and MeshXT packets
    if (meshPkt.portnum != PORTNUM_TEXT_MESSAGE_APP &&
        meshPkt.portnum != PORTNUM_PRIVATE_APP) {
        if (DEBUG_SERIAL) {
            Serial.printf("[Gateway] Ignoring portnum %d\n", meshPkt.portnum);
        }
        return;
    }

    // Translate to satellite format
    SatellitePacket satPkt;
    if (!_translator.toSatellite(meshPkt, satPkt)) {
        Serial.println("[Gateway] Translation failed, dropping packet.");
        return;
    }

    // Enqueue for next satellite pass
    if (enqueue(satPkt)) {
        Serial.printf("[Gateway] Queued message from 0x%08X (priority=%d, queue=%d/%d)\n",
            meshPkt.source, satPkt.priority, _queueCount, QUEUE_MAX_ENTRIES);

        // Blink LED to indicate queued message
        digitalWrite(LED_PIN, LOW);
        delay(50);
        digitalWrite(LED_PIN, HIGH);
    } else {
        Serial.println("[Gateway] Queue full! Message dropped.");
    }
}

void SatelliteGateway::handleSatellitePass() {
    if (_queueCount == 0) return;
    if (!_loraWAN.isJoined()) {
        // Try to join during pass
        _loraWAN.join();
        return;
    }

    QueueEntry entry;
    if (!dequeueHighestPriority(entry)) return;

    // Check duty cycle
    if (!_loraWAN.canTransmit(200)) {  // Estimate ~200ms per packet
        Serial.println("[Gateway] Duty cycle exhausted for this pass.");
        return;
    }

    // Send via LoRaWAN
    if (_loraWAN.send(entry.payload, entry.payloadLen, LORAWAN_FPORT)) {
        Serial.printf("[Gateway] Satellite TX OK: %d bytes (retries=%d)\n",
            entry.payloadLen, entry.retries);
    } else {
        // Re-enqueue with incremented retry count if retries remaining
        if (entry.retries < 3) {
            entry.retries++;
            if (_queueCount < QUEUE_MAX_ENTRIES) {
                _queue[_queueCount++] = entry;
            }
        } else {
            Serial.printf("[Gateway] Message 0x%08X dropped after max retries.\n", entry.id);
        }
    }

    // Small delay between transmissions
    delay(100);
}

void SatelliteGateway::handleDownlink() {
    DownlinkMessage dl = _loraWAN.getDownlink();
    if (dl.len == 0) return;

    Serial.printf("[Gateway] Processing downlink: %d bytes\n", dl.len);

    // Parse satellite packet
    SatellitePacket satPkt;
    if (!_translator.fromSatellite(dl.payload, dl.len, satPkt)) {
        Serial.println("[Gateway] Invalid downlink packet.");
        return;
    }

    // Convert to Meshtastic format and inject into mesh
    uint8_t meshBuf[MESHTASTIC_MAX_PACKET];
    uint16_t meshLen;
    if (!_translator.toMeshtastic(satPkt, meshBuf, meshLen)) {
        Serial.println("[Gateway] Downlink translation failed.");
        return;
    }

    if (_meshRx.transmit(meshBuf, meshLen)) {
        Serial.printf("[Gateway] Injected downlink into mesh: %d bytes -> 0x%08X\n",
            meshLen, satPkt.destNode);
    } else {
        Serial.println("[Gateway] Mesh injection failed.");
    }
}

bool SatelliteGateway::enqueue(const SatellitePacket &pkt) {
    if (_queueCount >= QUEUE_MAX_ENTRIES) return false;

    QueueEntry &entry = _queue[_queueCount];
    entry.id        = pkt.sourceNode ^ pkt.timestamp;  // Simple unique ID
    entry.priority  = pkt.priority;
    entry.timestamp = millis();
    entry.ttl       = ttlForPriority(pkt.priority);
    entry.retries   = 0;

    // Serialize the satellite packet
    _translator.serialize(pkt, entry.payload, entry.payloadLen);

    _queueCount++;
    return true;
}

bool SatelliteGateway::dequeueHighestPriority(QueueEntry &entry) {
    if (_queueCount == 0) return false;

    // Find highest priority (lowest number)
    uint8_t bestIdx = 0;
    uint8_t bestPriority = 255;
    for (uint8_t i = 0; i < _queueCount; i++) {
        if (_queue[i].priority < bestPriority) {
            bestPriority = _queue[i].priority;
            bestIdx = i;
        }
    }

    entry = _queue[bestIdx];

    // Remove from queue (shift remaining)
    for (uint8_t i = bestIdx; i < _queueCount - 1; i++) {
        _queue[i] = _queue[i + 1];
    }
    _queueCount--;

    return true;
}

void SatelliteGateway::purgeExpired() {
    uint32_t now = millis();
    uint8_t purged = 0;

    for (int i = _queueCount - 1; i >= 0; i--) {
        uint32_t age = (now - _queue[i].timestamp) / 1000;
        if (age > _queue[i].ttl) {
            // Remove expired entry
            for (uint8_t j = i; j < _queueCount - 1; j++) {
                _queue[j] = _queue[j + 1];
            }
            _queueCount--;
            purged++;
        }
    }

    if (purged > 0 && DEBUG_SERIAL) {
        Serial.printf("[Gateway] Purged %d expired messages.\n", purged);
    }
}

uint32_t SatelliteGateway::ttlForPriority(uint8_t priority) {
    switch (priority) {
        case PRIORITY_EMERGENCY: return TTL_EMERGENCY;
        case PRIORITY_HIGH:      return TTL_HIGH;
        case PRIORITY_NORMAL:    return TTL_NORMAL;
        case PRIORITY_LOW:       return TTL_LOW;
        default:                 return TTL_NORMAL;
    }
}

void SatelliteGateway::printStatus() {
    Serial.println("\n--- MeshXT-Satellite Status ---");
    Serial.printf("  Queue:     %d/%d messages\n", _queueCount, QUEUE_MAX_ENTRIES);
    Serial.printf("  LoRaWAN:   %s\n", _loraWAN.isJoined() ? "Joined" : "Not joined");
    Serial.printf("  Airtime:   %dms / %dms used\n",
                  _loraWAN.getAirtimeUsedMs(), DUTY_CYCLE_LIMIT_MS);
    Serial.printf("  Pass:      %s\n", _inPassWindow ? "ACTIVE" : "waiting");

    if (!_inPassWindow) {
        uint32_t untilPass = (_nextPassTime > millis()) ?
                             (_nextPassTime - millis()) / 60000 : 0;
        Serial.printf("  Next pass: ~%d minutes\n", untilPass);
    }
    Serial.println("-------------------------------\n");
}

// Arduino entry points
SatelliteGateway gateway;

void setup() {
    gateway.setup();
}

void loop() {
    gateway.loop();
}
