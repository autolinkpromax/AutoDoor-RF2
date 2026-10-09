#include "SYN480R.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <driver/gpio.h>
#include <esp_attr.h>
#define RF_DRAM DRAM_ATTR
#else
#define RF_DRAM
#endif

// Define static member variables in DRAM for safe interrupt execution during flash writes
int SYN480R::_rxPin = -1;
RF_DRAM volatile bool SYN480R::_available = false;
RF_DRAM volatile uint32_t SYN480R::_receivedValue = 0;
RF_DRAM volatile uint8_t SYN480R::_receivedBitLength = 0;
RF_DRAM volatile uint16_t SYN480R::_receivedPulseLength = 0;

RF_DRAM volatile uint32_t SYN480R::_lastTime = 0;
RF_DRAM volatile uint32_t SYN480R::_highDuration = 0;
RF_DRAM volatile uint32_t SYN480R::_lowDuration = 0;
RF_DRAM volatile bool SYN480R::_receiving = false;
RF_DRAM volatile uint32_t SYN480R::_tempCode = 0;
RF_DRAM volatile uint8_t SYN480R::_bitCount = 0;
RF_DRAM volatile uint32_t SYN480R::_lastFrameValue = 0;
RF_DRAM volatile uint32_t SYN480R::_lastFrameTimeUs = 0;


void SYN480R::begin(int rxPin) {
    _rxPin = rxPin;
    pinMode(_rxPin, INPUT);
    enableReceive();
}

bool SYN480R::available() {
    return _available;
}

uint32_t SYN480R::read() {
    noInterrupts();
    uint32_t val = _receivedValue;
    _available = false;
    interrupts();
    return val;
}

RFData SYN480R::readData() {
    RFData data;
    noInterrupts();
    data.value = _receivedValue;
    data.bitLength = _receivedBitLength;
    data.pulseLength = _receivedPulseLength;
    data.protocol = (_receivedBitLength == 24) ? 1 : (_receivedBitLength == 28 ? 2 : 3);
    _available = false;
    interrupts();
    return data;
}

uint32_t SYN480R::getReceivedValue() {
    return _receivedValue;
}

uint8_t SYN480R::getReceivedBitlength() {
    return _receivedBitLength;
}

uint16_t SYN480R::getReceivedDelay() {
    return _receivedPulseLength;
}

void SYN480R::resetAvailable() {
    noInterrupts();
    _available = false;
    interrupts();
}

bool SYN480R::isReceiving() {
    if (_receiving) {
        const uint32_t nowUs = micros();
        if ((uint32_t)(nowUs - _lastTime) > 100000) {
            _receiving = false;
        }
    }
    return _receiving;
}

void SYN480R::disableReceive() {
    if (_rxPin != -1) {
        detachInterrupt(digitalPinToInterrupt(_rxPin));
    }
}

void SYN480R::enableReceive() {
    if (_rxPin != -1) {
        _lastTime = micros();
        _receiving = false;
        _bitCount = 0;
        _tempCode = 0;
        _highDuration = 0;
        _lowDuration = 0;
        _lastFrameValue = 0;
        _lastFrameTimeUs = 0;
        pinMode(_rxPin, INPUT);
        attachInterrupt(digitalPinToInterrupt(_rxPin), handleInterrupt, CHANGE);
    }
}

#if __has_include("app/globals.h")
#include "app/globals.h"
#endif

namespace {

inline bool isNewFrameOrExpired(uint32_t code, uint32_t lastCode, uint32_t lastTimeUs, uint32_t currentTimeUs) {
#if __has_include("app/globals.h")
    if (gLearn.active) return true;
#endif
    if (code != lastCode) return true;
    if (lastTimeUs == 0) return true;
    const uint32_t diff = currentTimeUs - lastTimeUs;
    return (diff >= 15000);  // 15ms: debounce only rapid echo pulses, not legitimate burst repeats
}

}  // namespace

// Optimized Multi-Protocol ISR with Stale Timeout Reset & Non-Destructive Noise Filter
void IRAM_ATTR SYN480R::handleInterrupt() {
    const uint32_t currentTime = micros();
    const uint32_t duration = currentTime - _lastTime;

    // 0. Stale Frame Timeout Reset:
    // If more than 100ms (100,000us) has elapsed since the last edge,
    // any incomplete frame state from a previous press/drop is cleared immediately!
    if (duration > 100000) {
        _receiving = false;
        _bitCount = 0;
        _tempCode = 0;
    }

    // Clear stale frame cache if idle for more than 500ms
    if (duration > 500000) {
        _lastFrameValue = 0;
        _lastFrameTimeUs = 0;
    }

    // Non-destructive Glitch Filter:
    // Ignore spikes shorter than 30us (AGC thermal noise when no signal present)
    // Do not update _lastTime on spikes < 30us so pulse continuity across narrow glitches is preserved
    if (duration < 30) {
        return;
    }
    _lastTime = currentTime;

    int pinVal;
#if defined(ARDUINO_ARCH_ESP32)
    pinVal = gpio_get_level((gpio_num_t)_rxPin);
#else
    pinVal = digitalRead(_rxPin);
#endif

    if (pinVal == LOW) {
        // HIGH pulse just ended
        _highDuration = duration;
    } else {
        // LOW pulse just ended
        _lowDuration = duration;

        // 1. Sync Preamble / Gap Detection
        // Sync LOW ranges from 2,500us up to 28,000us.
        // Sync HIGH ranges from 35us up to 2,000us.
        if (_lowDuration >= 2500 && _lowDuration <= 28000 &&
            _highDuration >= 35 && _highDuration <= 2000 &&
            _lowDuration > (_highDuration * 2)) {

            // Publish completed multi-bit frame (12, 20, 24, 25, 26, 28, 30, 32 bits) right before this sync pulse
            if (_receiving && (_bitCount == 24 || _bitCount == 28 || _bitCount == 32 || _bitCount == 12 || _bitCount == 20 || _bitCount == 25 || _bitCount == 26 || _bitCount == 30)) {
                // Ignore noise patterns (0, all 1s)
                const uint32_t mask = (_bitCount == 32) ? 0xFFFFFFFF : ((1UL << _bitCount) - 1);
                if (_tempCode != 0 && (_tempCode & mask) != mask) {
                    if (isNewFrameOrExpired(_tempCode, _lastFrameValue, _lastFrameTimeUs, currentTime)) {
                        _lastFrameValue = _tempCode;
                        _lastFrameTimeUs = currentTime;

                        _receivedValue = _tempCode;
                        _receivedBitLength = _bitCount;
                        _available = true;
                        postRfCodeFromISR(_tempCode);
                    }
                }
            }

            _receiving = true;
            _bitCount = 0;
            _tempCode = 0;

            // Estimate clock period T from sync LOW (nominal EV1527 ratio ~31, HT6P20B ~23)
            uint16_t estimatedT = (uint16_t)((_lowDuration + 15) / 31);
            if (estimatedT < 80 || estimatedT > 1200) {
                estimatedT = (_highDuration >= 40 && _highDuration <= 1200) ? _highDuration : 350;
            }
            _receivedPulseLength = estimatedT;
            return;
        }

        // 2. Bit Demodulation (if currently receiving a frame)
        if (_receiving) {
            const uint32_t bitDuration = _highDuration + _lowDuration;

            // Nominal bit period is 4 * T.
            const uint32_t expected4T = _receivedPulseLength * 4;
            uint32_t minBitDur = (expected4T * 55) / 100;
            uint32_t maxBitDur = (expected4T * 160) / 100;

            if (minBitDur < 80) minBitDur = 80;
            if (maxBitDur > 5500) maxBitDur = 5500;

            if (bitDuration >= minBitDur && bitDuration <= maxBitDur) {
                // Adaptive clock tracking: smooth clock period T with actual bit durations
                const uint16_t sampleT = (uint16_t)(bitDuration / 4);
                if (sampleT >= 100 && sampleT <= 1000) {
                    _receivedPulseLength = (_receivedPulseLength == 0) ? sampleT : (uint16_t)((_receivedPulseLength * 3 + sampleT) / 4);
                }

                // Symmetrical Duty-Cycle Thresholding: 40% threshold accommodates pulse shrinking at longer range
                // (Nominal Bit 1 is 75% HIGH, nominal Bit 0 is 25% HIGH; 40% provides robust margin against shrinking down to 42%)
                const uint32_t highPercent = (_highDuration * 100) / bitDuration;
                const uint8_t bitVal = (highPercent >= 40) ? 1 : 0;

                _tempCode = (_tempCode << 1) | bitVal;
                _bitCount = _bitCount + 1;

                // Instant publish for standard 24-bit EV1527 / PT2262 frames
                if (_bitCount == 24) {
                    if (_tempCode != 0 && _tempCode != 0x00FFFFFF) {
                        if (isNewFrameOrExpired(_tempCode, _lastFrameValue, _lastFrameTimeUs, currentTime)) {
                            _lastFrameValue = _tempCode;
                            _lastFrameTimeUs = currentTime;

                            _receivedValue = _tempCode;
                            _receivedBitLength = 24;
                            _available = true;
                            postRfCodeFromISR(_tempCode);
                        }
                    }
                    _receiving = false;
                }
                // Instant publish when reaching 32 bits (max container capacity)
                else if (_bitCount == 32) {
                    if (_tempCode != 0 && _tempCode != 0xFFFFFFFF) {
                        if (isNewFrameOrExpired(_tempCode, _lastFrameValue, _lastFrameTimeUs, currentTime)) {
                            _lastFrameValue = _tempCode;
                            _lastFrameTimeUs = currentTime;

                            _receivedValue = _tempCode;
                            _receivedBitLength = 32;
                            _available = true;
                            postRfCodeFromISR(_tempCode);
                        }
                    }
                    _receiving = false;
                }
            } else {
                // Bit duration out of tolerance (corrupted by noise or inrush spike)
                // Discard invalid frame cleanly without publishing corrupted bits
                _receiving = false;
            }
        }
    }
}

__attribute__((weak)) void postRfCodeFromISR(uint32_t value) {
    (void)value;
}

