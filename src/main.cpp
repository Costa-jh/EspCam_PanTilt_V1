#include <Arduino.h>
#include <WiFi.h>
#include "camera.h"
#include "servo_control.h"
#include "web_server.h"
#include "config.h"
#include "tracking.h"
#include "distance.h"

static const char* WIFI_SSID     = "136";
static const char* WIFI_PASSWORD = "WIFITIME";

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi failed — restarting");
    ESP.restart();
  }
  Serial.println("\nConnected: http://" + WiFi.localIP().toString());

  setupCamera();
  setupServos();
  setupDistanceSensor();
  startServers();
}

void loop() {
  updateServos();
  readDistance();
  delay(1);
}