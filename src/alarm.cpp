#include "alarm.h"

#include "audio_player.h"
#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "router.h"

static bool alarmActive = false;
static int activeAlarmIndex = -1;
static unsigned long lastAlarmCheck = 0;

void initAlarm() { lastAlarmCheck = millis(); }

static void showAlarmUI() {
  char timeStr[64];
  sprintf(timeStr, "%s\n%02d:%02d\n%s", L(MSG_ALARM_TITLE),
          config.alarms[activeAlarmIndex].hour,
          config.alarms[activeAlarmIndex].minute, L(MSG_ALARM_STOP));
  showMessage(timeStr, 0, SHOW_WRAP);
}

void loopAlarm() {
  if (config.alarms.size() == 0) return;
  unsigned long now = millis();
  if (now - lastAlarmCheck < 1000) return;  // Check every second
  lastAlarmCheck = now;

  if (alarmActive) {
    showAlarmUI();
    if (!isAudioPlaying()) {
      audioPlayDefault(SOUND_ALARM);
    }
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  int h = timeinfo.tm_hour;
  int m = timeinfo.tm_min;
  int s = timeinfo.tm_sec;
  int wday = (timeinfo.tm_wday + 6) % 7;  // Convert to Mon=0, Sun=6

  if (s != 0) return;  // Only trigger at the start of the minute

  for (int i = 0; i < config.alarms.size(); i++) {
    auto& a = config.alarms[i];
    if (!a.enabled) continue;

    if (a.hour == h && a.minute == m) {
      bool trigger = false;

      // Repeat bitmask: 1=Mon, 2=Tue, 4=Wed, 8=Thu, 16=Fri, 32=Sat, 64=Sun,
      // 128=Once
      if (a.repeat == 0) {
        // Default: Daily
        trigger = true;
      } else if (a.repeat & (1 << wday)) {
        // Specific day
        trigger = true;
      } else if (a.repeat & 128) {
        // Once
        trigger = true;
        a.enabled = false;  // Disable after trigger
        saveConfig();
      }

      if (trigger) {
        alarmActive = true;
        activeAlarmIndex = i;
        if (!screenOn) toggleScreen();
        break;
      }
    }
  }
}

bool isAlarmActive() { return alarmActive; }

void stopAlarm() {
  alarmActive = false;
  activeAlarmIndex = -1;
  audioStop();
  // If it was a "once" alarm, we should disable it
  // But we don't have the repeat logic implemented yet
}
