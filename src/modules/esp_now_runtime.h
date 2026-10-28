#pragma once

#include <Arduino.h>

void initEspNow();
void disableEspNow();
void enableEspNow();
void ensureEspNowChannel();
void syncEspNowPeers();
void removeEspNowPeer(const String& mac);
bool startTagLearn();
void stopTagLearn();
void stepTagLearn();
void stepReceiverAnnounce();
void triggerImmediateAnnounce();
uint8_t currentEspNowChannel();
void checkEspNowReceiverHealth();
bool isEspNowReady();
void suspendEspNow();
void resumeEspNow();
void stepEspNowRxProcessing();
void deferEspNowBoot();          // Mark ESP-NOW init as pending (called from setup())
bool isEspNowBootPending();      // true until the first successful initEspNow()
void stepDeferredEspNowBoot();   // Called from loop(): init ESP-NOW after STA up / timeout
