#pragma once
#include <ESP32Servo.h>

void setupServos();
void updateServos();
void handleServoCommand(const char* cmd);