#include <WiFi.h>
#include <Wire.h>

#include "chronos_manager.h"
#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "weather.h"
#include "weather_icons.h"

#define OFFSET_DATE_X 2
#define OFFSET_DATE_Y 10
#define OFFSET_STATUSBAR_ICON_RIGHT_X 100
#define OFFSET_STATUSBAR_ICON_RIGHT_Y 10
#define OFFSET_TIME_X 2
#define OFFSET_TIME_Y 45
#define OFFSET_SEC_X 72
#define OFFSET_SEC_Y 35
#define OFFSET_WEATHER_ICON_X 90
#define OFFSET_WEATHER_ICON_Y 20
#define OFFSET_WEATHER_TEMP_X 90
#define OFFSET_WEATHER_TEMP_Y 55
#define OFFSET_HUM_X 110
#define OFFSET_HUM_Y 33
#define OFFSET_WIND_X 110
#define OFFSET_WIND_Y 42

static void drawStatusBar() {
  u8g2.setFont(u8g2_font_6x10_tf);

  String dateStr;
  const LocaleInfo* loc = getActiveLocale();
  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    char buffer[48];
    snprintf(buffer, sizeof(buffer), "%s, %02d%s", loc->days[timeinfo.tm_wday],
             timeinfo.tm_mday, loc->months[timeinfo.tm_mon]);
    dateStr = String(buffer);
  }
  u8g2.drawStr(OFFSET_DATE_X, OFFSET_DATE_Y, dateStr.c_str());

  // Bluetooth icon
  u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
  u8g2.drawGlyph(OFFSET_STATUSBAR_ICON_RIGHT_X, OFFSET_STATUSBAR_ICON_RIGHT_Y,
                 74);
  extern ChronosESP32 chronos;
  if (!config.bluetoothEnabled || !chronos.isConnected()) {
    u8g2.drawLine(OFFSET_STATUSBAR_ICON_RIGHT_X, OFFSET_STATUSBAR_ICON_RIGHT_Y,
                  OFFSET_STATUSBAR_ICON_RIGHT_X + 7,
                  OFFSET_STATUSBAR_ICON_RIGHT_Y - 7);
  }

  // WiFi icon
  u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
  u8g2.drawGlyph(OFFSET_STATUSBAR_ICON_RIGHT_X + 10,
                 OFFSET_STATUSBAR_ICON_RIGHT_Y, 0x0051);
  if (!config.wifiEnabled || WiFi.status() != WL_CONNECTED) {
    u8g2.drawLine(OFFSET_STATUSBAR_ICON_RIGHT_X + 10 + 1,
                  OFFSET_STATUSBAR_ICON_RIGHT_Y,
                  OFFSET_STATUSBAR_ICON_RIGHT_X + 12 + 7,
                  OFFSET_STATUSBAR_ICON_RIGHT_Y - 7);
  }
}

static void drawMainClock() {
  u8g2.setFont(u8g2_font_logisoso24_tn);
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
  u8g2.drawStr(OFFSET_TIME_X, OFFSET_TIME_Y, timeStr.c_str());

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(OFFSET_SEC_X, OFFSET_SEC_Y, secStr.c_str());
  u8g2.drawStr(OFFSET_SEC_X, OFFSET_SEC_Y + 10, ampmStr.c_str());

  // Weather icon
  uint8_t iconGlyph = 0x41;  // default cloudy
  int weatherCode = -1;
  bool isDay = true;

  if (WiFi.status() == WL_CONNECTED && onlineWeatherCode != -1) {
    weatherCode = onlineWeatherCode;
    isDay = onlineIsDay;
    const uint8_t* icon = icon_cloud_16x16;  // default

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

    u8g2.drawXBM(OFFSET_WEATHER_ICON_X, OFFSET_WEATHER_ICON_Y, 16, 16, icon_moon_16x16);
  }

  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
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
  u8g2.drawStr(OFFSET_WEATHER_TEMP_X, OFFSET_WEATHER_TEMP_Y, tempStr.c_str());

  // Sunrise / Sunset
  if (onlineSunrise != "" && onlineSunset != "") {
    u8g2.setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2.drawGlyph(2, 63, 0x45);  // sunrise
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(12, 63, onlineSunrise.c_str());

    u8g2.setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2.drawGlyph(65, 63, 0x44);  // sunset
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(75, 63, onlineSunset.c_str());
  }

  // Humidity and Wind
  u8g2.setFont(u8g2_font_5x7_tf);
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

  u8g2.drawStr(OFFSET_HUM_X, OFFSET_HUM_Y, humStr.c_str());
  u8g2.drawStr(OFFSET_WIND_X, OFFSET_WIND_Y, windStr.c_str());
}

void loopClock() {
  u8g2.clearBuffer();
  drawStatusBar();
  drawMainClock();
  u8g2.sendBuffer();
}