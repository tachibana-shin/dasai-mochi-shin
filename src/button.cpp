#include "button.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "display.h"
#include "wifi_manager.h"

static unsigned long lastTransitionTime = 0;
static unsigned long lastReleaseTime = 0;
static int buttonClicks = 0;
static bool lastRawState = HIGH;
static bool debouncedState = HIGH;
const unsigned long debounceDelay = 25;
const unsigned long multiClickDelay = 500;

static void handleClick() {
  Serial.println("Clicked!");
  config.brightness += 50;
  if (config.brightness > 255) config.brightness = 255;
  u8g2.setContrast(config.brightness);
  saveConfig();
}

static void handleDoubleclick() {
  Serial.println("Double clicked!");
  config.brightness -= 50;
  if (config.brightness < 5) config.brightness = 5;
  u8g2.setContrast(config.brightness);
  saveConfig();
}

static void handleTripleClick() {
  Serial.println("Triple clicked!");
  config.wifiEnabled = !config.wifiEnabled;
  saveConfig();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  if (config.wifiEnabled) {
    u8g2.drawStr(0, 20, "WiFi ON");
    u8g2.sendBuffer();
    delay(1000);
    connectWiFi();
  } else {
    u8g2.drawStr(0, 20, "WiFi OFF");
    u8g2.sendBuffer();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(1000);
  }
}

void initButton() { pinMode(config.pinSensorTap, INPUT_PULLUP); }

void loopButton() {
  bool rawState = digitalRead(config.pinSensorTap);

  if (rawState != lastRawState) {
    lastTransitionTime = millis();
  }
  lastRawState = rawState;

  if ((millis() - lastTransitionTime) > debounceDelay) {
    if (rawState != debouncedState) {
      debouncedState = rawState;
      if (debouncedState == HIGH) {
        if (!screenOn) {
          toggleScreen();
          buttonClicks = 0;
        } else {
          buttonClicks++;
          lastReleaseTime = millis();
        }
      }
    }
  }

  if (buttonClicks > 0 && (millis() - lastReleaseTime > multiClickDelay)) {
    if (buttonClicks == 1)
      handleClick();
    else if (buttonClicks == 2)
      handleDoubleclick();
    else if (buttonClicks == 3)
      handleTripleClick();
    else if (buttonClicks >= 4)
      toggleScreen();
    buttonClicks = 0;
  }
}