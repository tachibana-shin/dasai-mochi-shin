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

struct AlarmEntry {
  bool enabled = false;
  int hour = 7;
  int minute = 0;
  uint8_t repeat = 0;  // Bitmask: 1=Mon, 2=Tue, 4=Wed, 8=Thu, 16=Fri, 32=Sat,
                       // 64=Sun, 128=Once
};

struct DrinkConfig {
  bool enabled = false;
  int startHour = 8;
  int endHour = 20;
  int intervalMinutes = 60;
  int durationSeconds = 30;
  float dailyGoalLiters = 2.0;
};

struct AudioConfig {
  bool alarmEnabled = true;
  bool drinkEnabled = true;
  bool notifyEnabled = true;
  int volume = 12;  // 0-21
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

  int pinAudioLRC = 5;
  int pinAudioDIN = 7;
  int pinAudioBCLK = 8;

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

  String homePath = "/DasaiMochi";

  // config for Dasai Mochi mode
  uint16_t mochiSpeedDivisor = 1;
  bool mochiNegative = false;
  unsigned long mochiClockInterval = 180000;  // Default 3 mins
  unsigned long mochiClockDuration = 5000;    // Default 5 secs

  bool screenFlipMode = false;
  bool screenNegative = false;
  uint16_t screenWidth = 128;
  uint16_t screenHeight = 64;

  String customClickSoundPath = "Audio/click.qwav";
  String customDrinkSoundPath = "Audio/drink.qwav";
  String customAlarmSoundPath = "Audio/alarm.qwav";
  String customNotifySoundPath = "Audio/notify.qwav";

  std::vector<AlarmEntry> alarms;
  DrinkConfig drink;
  AudioConfig audio;

  void fromJson(const JsonObject& doc);
  void fromJsonBoot(const JsonObject& doc);
  void toJson(JsonDocument& doc) const;
  void toJsonBoot(JsonDocument& doc) const;
  void debugPrint() const;
};

extern AppConfig config;

bool loadConfig();
bool loadBootConfig();
bool saveConfig();
bool saveBootConfig();

#endif