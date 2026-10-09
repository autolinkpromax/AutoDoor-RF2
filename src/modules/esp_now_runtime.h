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
