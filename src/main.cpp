#include <Arduino.h>
#include <ChronosESP32.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "button.h"
#include "chronos_manager.h"
#include "clock.h"
#include "config.h"
#include "display.h"
#include "filesystem.h"
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
}

void loop() {
  loopButton();
  loopChronos();
  // display module only check auto on/off device not is function show route
  loopDisplay();

  if (!isPortalActive) {
    loopWeather();
    loopClock();
  }

  delay(10);
}