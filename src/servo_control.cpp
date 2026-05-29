#include "servo_control.h"
#include "config.h"

Servo panServo;
Servo tiltServo;

static unsigned long panStopAt  = 0;
static unsigned long tiltStopAt = 0;

void setupServos() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);

    panServo.setPeriodHertz(50);
    tiltServo.setPeriodHertz(50);
    panServo.attach(PAN_SERVO_PIN, 500, 2500);
    tiltServo.attach(TILT_SERVO_PIN, 500, 2500);
    panServo.write(90);
    tiltServo.write(90);

    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
}

void updateServos() {
    unsigned long now = millis();
    if (panStopAt && now >= panStopAt) {
        panServo.write(90);
        panStopAt = 0;
    }
    if (tiltStopAt && now >= tiltStopAt) {
        tiltServo.write(90);
        tiltStopAt = 0;
    }
}

void handleServoCommand(const char* cmd) {
    if (strcmp(cmd, "PAN:LEFT") == 0) {
        panServo.write(100);
        panStopAt = millis() + MOVE_COOLDOWN;
    } else if (strcmp(cmd, "PAN:RIGHT") == 0) {
        panServo.write(80);
        panStopAt = millis() + MOVE_COOLDOWN;
    } else if (strcmp(cmd, "TILT:UP") == 0) {
        tiltServo.write(80);
        tiltStopAt = millis() + MOVE_COOLDOWN;
    } else if (strcmp(cmd, "TILT:DOWN") == 0) {
        tiltServo.write(100);
        tiltStopAt = millis() + MOVE_COOLDOWN;
    } else if (strcmp(cmd, "LIGHT:ON") == 0) {
        digitalWrite(4, HIGH);
    } else if (strcmp(cmd, "LIGHT:OFF") == 0) {
        digitalWrite(4, LOW);
    }
}