#include "button.h"
#include "audio_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "alarm.h"
#include "config.h"
#include "display.h"
#include "menu.h"
#include "reminder.h"
#include "router.h"
#include "wifi_manager.h"

static unsigned long lastTransitionTime = 0;
static unsigned long lastReleaseTime = 0;
static int buttonClicks = 0;
static bool lastRawState = HIGH;
static bool debouncedState = HIGH;
const unsigned long debounceDelay = 25;
const unsigned long multiClickDelay = 500;

static void handleClick() {
  if (isAlarmActive()) {
    stopAlarm();
    return;
  }
  Serial.println("click");
  playTapAudio();
  if (isReminderActive()) {
    confirmDrink();
    return;
  }
  if (Router::current() == Route::SETTINGS) {
    handleMenuClick();
    return;
  }
  toggleScreen();
}

static void handleDoubleclick() {
  playTapAudio();
  if (Router::current() == Route::SETTINGS) {
    handleMenuDoubleClick();
    return;
  }

  Serial.println("double click");
  if (Router::current() == Route::DASAI_MOCHI) {
    Serial.println("DASAI_MOCHI");
    Router::replace(Route::CLOCK);
  } else if (Router::current() == Route::CLOCK) {
    Serial.println("CLOCK");
    Router::replace(Route::DASAI_MOCHI);
  }
}

static void handleTripleClick() {
  playTapAudio();
  if (Router::current() == Route::SETTINGS) {
    handleMenuTripleClick();
    return;
  }
  Router::push(Route::SETTINGS);
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
      ESP.restart();  // Let's use 4 clicks for restart or something useful
    buttonClicks = 0;
  }
}