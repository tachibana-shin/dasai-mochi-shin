#include <Arduino.h>
#include <ChronosESP32.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "alarm.h"
#include "audio_player.h"
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
#include "web_usb.h"
#include "wifi_manager.h"

void webUsbTask(void *pvParameters) {
  for (;;) {
    loopWebUsb();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void inputTask(void *pvParameters) {
  for (;;) {
    loopButton();
    vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz check for buttons
  }
}

void mainTask(void *pvParameters) {
  for (;;) {
    loopChronos();
    loopDisplay();
    loopReminder();
    loopAlarm();

    if (isShowingMessage()) {
      u8g2->clearBuffer();
      drawMessageContent();
      sendBuffer();
    } else if (!isAlarmActive() && !isReminderActive()) {
      Router::loop();
    }

    vTaskDelay(pdMS_TO_TICKS(20));  // ~50 FPS, energy efficient
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  setCpuFrequencyMhz(80);  // Power saving mode

  struct timeval tv;
  tv.tv_sec = 1767225600;  // Unix timestamp

  configTime(config.gmtOffset_sec, config.daylightOffset_sec, nullptr);
  settimeofday(&tv, NULL);

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
  audioInit();

  initDasaiMochi();

  Serial.println("Setup complete");

  // Create tasks (No pinning needed for single-core ESP32-C3)
  xTaskCreate(webUsbTask, "WebUsbTask", 4096, NULL, 2, NULL);
  xTaskCreate(inputTask, "InputTask", 4096, NULL, 5, NULL);
  xTaskCreate(mainTask, "MainTask", 8192, NULL, 1, NULL);
}

void loop() {
  vTaskDelete(NULL);  // Delete the default loop task
}