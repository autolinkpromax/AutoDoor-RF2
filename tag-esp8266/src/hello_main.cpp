#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Hello World from ESP8285 / ESP-01!");
}

void loop() {
  Serial.println("Running...");
  delay(2000);
}
