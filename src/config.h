#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>

#include <vector>

extern SPIClass sdSPI;

struct WifiEntry {
  String ssid;
  String pass;
};

struct AppConfig {
  int brightness = 150;
  bool wifiEnabled = true;
  bool bluetoothEnabled = true;
  unsigned long weatherInterval = 1800000;
  int autoOffHour = 23;
  int autoOnHour = 7;

  std::vector<WifiEntry> wifi;

  // pin settings
  int pinScreenSDA = 20;
  int pinScreenSCL = 21;
  int pinSensorTap = 10;
  int pinSdCS = 4;
  int pinSdMOSI = 3;
  int pinSdCLK = 2;
  int pinSdMISO = 1;

  String wifiAPName = "Dasai Mochi Shin";
  String bluetoothName = "Mochi Shin";
  String ntpServer = "time.google.com";

  long gmtOffset_sec = 7 * 3600;
  int daylightOffset_sec = 0;
  bool is24Hour = false;
  String langCode = "vi";

  String weatherServer =
      "http://api.open-meteo.com/v1/forecast?"
      "latitude=10.762622&longitude=106.660172&"
      "current=temperature_2m,relative_humidity_2m,wind_speed_10m,"
      "weather_code,is_day&daily=sunrise,sunset&timezone=auto";

  String configPath = "/config.json";

  void fromJson(const JsonObject& doc);
  void toJson(JsonDocument& doc) const;
  void debugPrint() const;
};

extern AppConfig config;

bool loadConfig();
bool saveConfig();

#endif