/**
 * MeshXT-Satellite — Store & Forward Example
 * 
 * Queues messages from the mesh and sends them during satellite passes.
 * Messages persist in flash memory and survive power cycles.
 */

#include "../src/gateway/SatelliteGateway.h"
#include "../src/gateway/config.h"

SatelliteGateway gateway;

void setup() {
    Serial.begin(115200);
    Serial.println("MeshXT-Satellite Store & Forward Gateway v0.1.0");
    
    if (!gateway.begin()) {
        Serial.println("ERROR: Gateway init failed");
        while (1) delay(1000);
    }
    
    // Load any queued messages from flash
    int queued = gateway.getQueueSize();
    if (queued > 0) {
        Serial.printf("Restored %d queued messages from flash\n", queued);
    }
    
    Serial.println("Gateway ready. Messages will be sent on next satellite pass.");
}

void loop() {
    gateway.update();
    
    // Print status every 60 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 60000) {
        lastStatus = millis();
        Serial.printf("[Status] Queue: %d msgs | Next pass: %s\n",
            gateway.getQueueSize(),
            gateway.isInSatellitePass() ? "NOW" : "waiting"
        );
    }
    
    delay(10);
}
