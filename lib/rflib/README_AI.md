# SYN480R Library (AI-Optimized Spec)

Lightweight, non-blocking ASK/OOK 433.92MHz RF receiver library for Arduino/ESP32. Decodes EV1527/PT2262.

## Key Features
- **Static Class:** No instantiation needed. Faster ISR due to direct memory access.
- **Hardware-Optimized:** Direct GPIO read (`gpio_get_level`) on ESP32; falls back to `digitalRead()`.
- **Noise Filtering:** Discards glitches < 80us. Detects missed edges via state tracking.
- **Ratio Check:** Validates 1:3 vs 3:1 pulse ratio (T midpoint threshold = 1.5).

## API Reference

```cpp
#include <SYN480R.h>

struct RFData {
    uint32_t value;       // Decoded code (24-bit for EV1527)
    uint8_t bitLength;    // Decoded bit count (12 to 32)
    uint16_t pulseLength; // Estimated unit pulse T in microseconds
    uint8_t protocol;     // Decoded protocol (always 1)
};

class SYN480R {
public:
    static void begin(int rxPin);       // Start receiver on GPIO pin
    static bool available();            // Check for new decoded data
    static uint32_t read();             // Read value and auto-reset available flag
    static RFData readData();           // Read all details and auto-reset flag
    static void resetAvailable();       // Manually reset available flag
    static void disableReceive();       // Stop interrupt
    static void enableReceive();        // Resume interrupt
};
```

## Quick Example (ESP32-C3, Pin 2)

```cpp
#include <Arduino.h>
#include <SYN480R.h>

void setup() {
    Serial.begin(115200);
    SYN480R::begin(2);
}

void loop() {
    if (SYN480R::available()) {
        RFData data = SYN480R::readData(); // auto-resets available flag
        Serial.printf("Value: 0x%08X, Bits: %d, T: %dus\n", data.value, data.bitLength, data.pulseLength);
    }
}
```
