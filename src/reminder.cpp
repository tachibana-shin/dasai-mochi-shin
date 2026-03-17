#include "reminder.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <WiFi.h>

#include "audio_manager.h"
#include "chronos_manager.h"
#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "filesystem.h"
#include "time_utils.h"

static unsigned long lastReminderTime = 0;
static bool reminderActive = false;
static unsigned long reminderStartTime = 0;
static int missedReminders = 0;
static unsigned long lastLoopCheck = 0;

void initReminder() { lastReminderTime = millis(); }

void recordDrinkEvent(String type, float amount) {
  JsonDocument doc;
  struct tm timeinfo;
  char timeStr[32];

  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  } else {
    return;
  }

  doc["ts"] = timeStr;
  doc["type"] = type;
  if (amount > 0) doc["amount"] = amount;

  String jsonl;
  serializeJson(doc, jsonl);
  jsonl += "\n";

  fs::File file =
      getFile(config.homePath + "/drink_log.jsonl", FILE_APPEND, true);
  if (file) {
    file.print(jsonl);
    file.close();
  }
}

static void showDrinkPopup() {
  String msg = String(L(MSG_DRINK_NOW)) + "\n" + L(MSG_DRINK_DESC) + "\n" +
               L(MSG_DRINK_CONFIRM);
  showMessage(msg.c_str(), 0, SHOW_WRAP);
}

void loopReminder() {
  if (!config.drink.enabled) return;

  unsigned long now = millis();
  if (now - lastLoopCheck < 1000) return;
  lastLoopCheck = now;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  int currentHour = timeinfo.tm_hour;

  if (currentHour < config.drink.startHour ||
      currentHour >= config.drink.endHour)
    return;

  unsigned long intervalMs =
      (unsigned long)config.drink.intervalMinutes * 60 * 1000;

  if (!reminderActive && (now - lastReminderTime > intervalMs)) {
    reminderActive = true;
    reminderStartTime = now;
    recordDrinkEvent("reminder_start");

    if (!screenOn) toggleScreen();
    showDrinkPopup();
    playDrinkAudio();
  }

  if (reminderActive) {
    showDrinkPopup();
    if (now - reminderStartTime >
        (unsigned long)config.drink.durationSeconds * 1000) {
      reminderActive = false;
      lastReminderTime = now;
      missedReminders++;
      recordDrinkEvent("miss");
      showMessage(L(MSG_MISSED_DRINK), 2000);
    }

    // Handle confirmation in button logic by calling recordDrinkEvent("drink",
    // 200)
  }
}

// In button.cpp we should check if reminderActive and intercept click
bool isReminderActive() { return reminderActive; }
void confirmDrink() {
  reminderActive = false;
  lastReminderTime = millis();
  recordDrinkEvent("drink", 200);  // Default 200ml
  showMessage(L(MSG_DRINK_GREAT), 2000);
}

int getMissedReminders() { return missedReminders; }
void resetMissedReminders() {
  if (missedReminders > 0) {
    recordDrinkEvent("manual_fix_miss");
    missedReminders = 0;
  }
}
