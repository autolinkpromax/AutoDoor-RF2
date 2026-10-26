#pragma once

#include <Arduino.h>

void loadWebAuthFromPrefs();
String createWebSession();
void revokeAllWebSessions();
bool cookieSessionValid();
bool isWebAuthed();
bool ensureWebAuth();
void clearWebAuthPassword();
