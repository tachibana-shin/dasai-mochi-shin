#include <WiFi.h>
#include <Wire.h>

#include "chronos_manager.h"
#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "reminder.h"
#include "weather.h"
#include "weather_icons.h"

void drawStatusBar() {
  u8g2->setFont(u8g2_font_6x10_tf);

  String dateStr;
  const LocaleInfo* loc = getActiveLocale();
  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    char buffer[48];
    snprintf(buffer, sizeof(buffer), "%s, %02d%s", loc->days[timeinfo.tm_wday],
             timeinfo.tm_mday, loc->months[timeinfo.tm_mon]);
    dateStr = String(buffer);
  }
  u8g2->drawStr(vw(1.5), vh(15.6), dateStr.c_str());

  int xPos = vw(100);
  int yPos = vh(15.6);
  int spacing = rem(0.4); // Spacing between items

  // 1. Battery Icon & Text
  u8g2->setFont(u8g2_font_open_iconic_embedded_1x_t);
  xPos -= 10; // Battery icon width approx 10
  u8g2->drawGlyph(xPos, yPos, 73); // battery icon
  
  u8g2->setFont(u8g2_font_5x7_tf);
  uint8_t battery = chronos.getPhoneBattery();
  char battBuf[8];
  sprintf(battBuf, "%d%%", battery);
  int battWidth = u8g2->getStrWidth(battBuf);
  xPos -= (battWidth + 2);
  u8g2->drawStr(xPos, yPos - 1, battBuf);
  xPos -= spacing;

  // 2. WiFi Icon
  bool wifiOk = config.wifiEnabled && WiFi.status() == WL_CONNECTED;
  u8g2->setFont(u8g2_font_open_iconic_www_1x_t);
  xPos -= 10;
  u8g2->drawGlyph(xPos, yPos, 0x0051);
  if (!wifiOk) {
    u8g2->drawLine(xPos - 1, yPos, xPos + 9, yPos - 8);
  }
  xPos -= spacing;

  // 3. Bluetooth Icon
  bool bleOk = config.bluetoothEnabled && chronos.isConnected();
  u8g2->setFont(u8g2_font_open_iconic_embedded_1x_t);
  xPos -= 10;
  u8g2->drawGlyph(xPos, yPos, 74);
  if (!bleOk) {
    u8g2->drawLine(xPos - 1, yPos, xPos + 9, yPos - 8);
  }
  xPos -= spacing;

  // 4. Drink Reminder Icon (Drop) - only if missed > 0
  if (getMissedReminders() > 0) {
    bool flash = (millis() / 500) % 2 == 0;
    if (flash) {
      u8g2->setFont(u8g2_font_open_iconic_weather_1x_t);
      xPos -= 10;
      u8g2->drawGlyph(xPos, yPos, 0x48); // drop icon
      xPos -= spacing;
    } else {
      xPos -= (10 + spacing); // Keep space even when hidden during flash
    }
  }

  // 5. Alarm Icon
  bool anyAlarm = false;
  for (const auto& a : config.alarms) {
    if (a.enabled) {
      anyAlarm = true;
      break;
    }
  }
  if (anyAlarm) {
    u8g2->setFont(u8g2_font_open_iconic_embedded_1x_t);
    xPos -= 10;
    u8g2->drawGlyph(xPos, yPos, 65);
    xPos -= spacing;
  }
}

void drawMainClock() {
  u8g2->setFont(u8g2_font_logisoso24_tn);
  String hourStr, minuteStr, secStr, ampmStr;

  struct tm timeinfo;
  bool timeValid = false;
  if (getLocalTime(&timeinfo)) {
    timeValid = true;
    char h[3], m[3], s[3], p[3];
    if (config.is24Hour) {
      strftime(h, sizeof(h), "%H", &timeinfo);
      ampmStr = "";
    } else {
      strftime(h, sizeof(h), "%I", &timeinfo);
      strftime(p, sizeof(p), "%p", &timeinfo);
      ampmStr = String(p);
    }

    strftime(m, sizeof(m), "%M", &timeinfo);
    strftime(s, sizeof(s), "%S", &timeinfo);

    hourStr = String(h);
    minuteStr = String(m);
    secStr = String(s);
  }

  const LocaleInfo* loc = getActiveLocale();
  if (ampmStr == "AM")
    ampmStr = loc->am;
  else if (ampmStr == "PM")
    ampmStr = loc->pm;

  String timeStr = hourStr + ":" + minuteStr;
  u8g2->drawStr(vw(1.5), vh(70.3), timeStr.c_str());

  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->drawStr(vw(56.2), vh(54.6), secStr.c_str());
  u8g2->drawStr(vw(56.2), vh(54.6) + 10, ampmStr.c_str());

  // Weather icon
  uint8_t iconGlyph = 0x41;  // default cloudy
  int weatherCode = -1;
  bool isDay = true;

  const uint8_t* icon = icon_cloud_16x16;  // default
  if (WiFi.status() == WL_CONNECTED && onlineWeatherCode != -1) {
    weatherCode = onlineWeatherCode;
    isDay = onlineIsDay;
  } else {
    weatherCode = chronos.getWeatherAt(0).icon;
  }

  if (weatherCode == 0)
    icon = isDay ? icon_sun_16x16 : icon_moon_16x16;
  else if (weatherCode == 1 || weatherCode == 2)
    icon = icon_cloud_16x16;
  else if (weatherCode == 3 || weatherCode == 45 || weatherCode == 48)
    icon = icon_cloud_16x16;
  else if ((weatherCode >= 51 && weatherCode <= 65) ||
           (weatherCode >= 80 && weatherCode <= 82))
    icon = icon_rain_16x16;
  else if ((weatherCode >= 71 && weatherCode <= 77) ||
           (weatherCode >= 85 && weatherCode <= 86))
    icon = icon_snow_16x16;
  else if (weatherCode >= 95)
    icon = icon_thunder_16x16;

  u8g2->drawXBM(vw(70.3), vh(31.2), 16, 16, icon);

  u8g2->setFont(u8g2_font_unifont_t_vietnamese1);
  String tempStr = "??";
  int humidity = -1;
  float wind = -1;

  if (WiFi.status() == WL_CONNECTED && onlineTemp != -999) {
    tempStr = String(onlineTemp);
    humidity = onlineHumidity;
    wind = onlineWindSpeed;
  } else {
    extern ChronosESP32 chronos;
    if (chronos.getWeatherCount() > 0) {
      tempStr = String(chronos.getWeatherAt(0).temp);
    }
  }

  if (tempStr != "??") tempStr += "C";
  u8g2->drawStr(vw(70.3), vh(85.9), tempStr.c_str());

  // Sunrise / Sunset
  if (onlineSunrise != "" && onlineSunset != "") {
    u8g2->setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2->drawGlyph(vw(1.5), vh(98.4), 0x45);  // sunrise
    u8g2->setFont(u8g2_font_5x7_tf);
    u8g2->drawStr(vw(9.3), vh(98.4), onlineSunrise.c_str());

    u8g2->setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2->drawGlyph(vw(50.7), vh(98.4), 0x44);  // sunset
    u8g2->setFont(u8g2_font_5x7_tf);
    u8g2->drawStr(vw(58.5), vh(98.4), onlineSunset.c_str());
  }

  // Humidity and Wind
  u8g2->setFont(u8g2_font_5x7_tf);
  String humStr = "H";
  String windStr = "W";

  if (humidity != -1)
    humStr += String(humidity) + "%";
  else
    humStr += "??";

  if (wind != -1)
    windStr += String(wind, 1) + "m/s";
  else
    windStr += "??";

  u8g2->drawStr(vw(85.9), vh(51.5), humStr.c_str());
  u8g2->drawStr(vw(85.9), vh(65.6), windStr.c_str());
}

void loopClock() {
  u8g2->clearBuffer();
  drawStatusBar();
  drawMainClock();
  sendBuffer();
}