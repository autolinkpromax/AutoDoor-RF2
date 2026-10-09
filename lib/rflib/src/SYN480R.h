#ifndef SYN480R_H
#define SYN480R_H

#include <Arduino.h>

struct RFData {
    uint32_t value;       // Decoded data value
    uint8_t bitLength;    // Number of bits decoded
    uint16_t pulseLength; // Estimated unit pulse length (T) in microseconds
    uint8_t protocol;     // Protocol ID (1 for standard EV1527/PT2262)
};

class SYN480R {
public:
    // Initialize receiver on specified pin
    static void begin(int rxPin);
    
    // Check if new data is available
    static bool available();
    
    // Retrieve decoded value and clear the available flag automatically
    static uint32_t read();
    
    // Retrieve all decoded data details and clear available flag automatically
    static RFData readData();
    
    // Retrieve decoded value only (does NOT clear available flag)
    static uint32_t getReceivedValue();
    
    // Retrieve decoded bit length only
    static uint8_t getReceivedBitlength();
    
    // Retrieve estimated unit pulse width (delay) in microseconds
    static uint16_t getReceivedDelay();
    
    // Clear available flag manually if using getReceivedValue()
    static void resetAvailable();
    
    // Check if 433MHz receiver is currently receiving an RF frame
    static bool isReceiving();

    // Disable/Enable the pin interrupt
    static void disableReceive();
    static void enableReceive();

private:
    static int _rxPin;
    
    // ISR handler function (marked with IRAM_ATTR for ESP32/ESP8266 compatibility)
    static void IRAM_ATTR handleInterrupt();

    // Shared volatile variables between ISR and main thread
    static volatile bool _available;
    static volatile uint32_t _receivedValue;
    static volatile uint8_t _receivedBitLength;
    static volatile uint16_t _receivedPulseLength;
    
    // ISR-specific state variables
    static volatile uint32_t _lastTime;
    static volatile uint32_t _highDuration;
    static volatile uint32_t _lowDuration;
    static volatile bool _receiving;
    static volatile uint32_t _tempCode;
    static volatile uint8_t _bitCount;
    static volatile uint32_t _lastFrameValue;
    static volatile uint32_t _lastFrameTimeUs;
};

void postRfCodeFromISR(uint32_t value);

#endif // SYN480R_H
