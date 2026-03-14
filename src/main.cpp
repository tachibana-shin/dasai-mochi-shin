#include <Arduino.h>
#include <ChronosESP32.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "alarm.h"
#include "button.h"
#include "chronos_manager.h"
#include "clock.h"
#include "config.h"
#include "dasai_mochi.h"
#include "display.h"
#include "filesystem.h"
#include "reminder.h"
#include "router.h"
#include "time_utils.h"
#include "weather.h"
#include "wifi_manager.h"
#include "audio_manager.h"

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  initFFS();
  loadBootConfig();
  initFilesystem();
  loadConfig();
  initDisplay();
  initButton();
  initChronos();
  initWiFi();
  initReminder();
  initAlarm();
  initAudio();
  // boot image in initDasaiMochi
  initDasaiMochi();
}

void loop() {
  loopButton();
  loopChronos();
  // display module only check auto on/off device
  loopDisplay();
  loopReminder();
  loopAlarm();
  loopAudio();

  // draw router
  Router::loop();

  delay(10);
}