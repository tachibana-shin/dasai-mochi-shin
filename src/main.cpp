#include <Arduino.h>
#include <ChronosESP32.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "button.h"
#include "chronos_manager.h"
#include "clock.h"
#include "config.h"
#include "dasai_mochi.h"
#include "display.h"
#include "filesystem.h"
#include "router.h"
#include "time_utils.h"
#include "weather.h"
#include "wifi_manager.h"

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  loadBootConfig();
  initFilesystem();
  loadConfig();
  initDisplay();
  initButton();
  initWiFi();
  initChronos();
  initDasaiMochi();
}

void loop() {
  loopButton();
  loopChronos();
  // display module only check auto on/off device not is function show route
  loopDisplay();

  // draw router
  Router::loop();

  delay(10);
}