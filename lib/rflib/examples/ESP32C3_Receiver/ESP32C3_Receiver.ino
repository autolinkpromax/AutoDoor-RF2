#include <Arduino.h>
#include <SYN480R.h>

// Pin mapping: GPIO 2 is used for SYN480R DATA connection on ESP32-C3
const int RX_PIN = 2;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10); // Wait for Serial Monitor on USB CDC
    }
    
    Serial.println("\n==================================");
    Serial.println("SYN480R RF433 Receiver Demo (Static API)");
    Serial.println("==================================");
    
    // Start the receiver
    SYN480R::begin(RX_PIN);
    Serial.printf("Receiver started on GPIO %d\n", RX_PIN);
    Serial.println("Listening for EV1527/PT2262 signals...");
}

void loop() {
    // readData() automatically retrieves data and resets the available flag for the next packet.
    if (SYN480R::available()) {
        RFData rfData = SYN480R::readData();
        
        Serial.println("\n--- New Signal Received ---");
        Serial.printf("Value (HEX): 0x%08X\n", rfData.value);
        Serial.printf("Value (DEC): %u\n", rfData.value);
        Serial.printf("Bit Length : %d bits\n", rfData.bitLength);
        Serial.printf("Pulse (T)  : %d us\n", rfData.pulseLength);
        Serial.printf("Protocol   : %d\n", rfData.protocol);
    }
}
