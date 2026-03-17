#include "time_utils.h"

#include <ChronosESP32.h>
#include <WiFi.h>
#include <sys/time.h>
#include <time.h>

#include "chronos_manager.h"
#include "config.h"

bool ntpSynced = false;
bool syncNTP() {
  if (ntpSynced || chronos.isConnected()) {
    ntpSynced = true;
    return true;
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(config.gmtOffset_sec, config.daylightOffset_sec,
               config.ntpServer.c_str());
    Serial.println("NTP sync started");
    ntpSynced = true;
    return true;
  }
  return false;
}

void syncLocalFromChronos() {
  struct tm t;
  memset(&t, 0, sizeof(t));

  t.tm_year = chronos.getYear() - 1900;  // 2026
  t.tm_mon = chronos.getMonth();         // 2
  t.tm_mday = chronos.getDay();

  int hour = chronos.getHour();

  if (!chronos.is24Hour()) {
    String ampm = chronos.getAmPmC(true);

    if (ampm == "PM" && hour != 12) hour += 12;
    if (ampm == "AM" && hour == 12) hour = 0;
  }

  t.tm_hour = hour;
  t.tm_min = chronos.getMinute();
  t.tm_sec = chronos.getSecond();

  time_t now = mktime(&t);
  struct timeval tv;
  tv.tv_sec = now;
  tv.tv_usec = 0;

  settimeofday(&tv, nullptr);
  }

  int getHour24() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    return timeinfo.tm_hour;
  }

  int hour = chronos.getHour();
  if (!chronos.is24Hour()) {
    String ampm = chronos.getAmPmC(true);
    if (ampm == "PM" && hour != 12) hour += 12;
    if (ampm == "AM" && hour == 12) hour = 0;
  }
  return hour;
  }