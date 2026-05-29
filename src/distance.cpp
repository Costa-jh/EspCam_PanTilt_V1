#include "distance.h"
#define TRIG_PIN 14
#define ECHO_PIN  15

float lastDistance = 0.0;

void setupDistanceSensor() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    lastDistance = -1;
  } else {
    lastDistance = ((duration / 2.0) * 0.0343) / 100.0;
  }
  return lastDistance;
}