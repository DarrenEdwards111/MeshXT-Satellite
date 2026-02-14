/**
 * MeshXT-Satellite — Basic Gateway Example
 * 
 * Minimal working example: receives Meshtastic messages on radio 1,
 * compresses with MeshXT, and sends via LoRaWAN on radio 2.
 */

#include "../src/gateway/SatelliteGateway.h"
#include "../src/gateway/config.h"

SatelliteGateway gateway;

void setup() {
    Serial.begin(115200);
    Serial.println("MeshXT-Satellite Gateway v0.1.0");
    
    if (!gateway.begin()) {
        Serial.println("ERROR: Gateway init failed");
        while (1) delay(1000);
    }
    
    Serial.println("Gateway ready. Listening for mesh traffic...");
}

void loop() {
    gateway.update();
    delay(10);
}
