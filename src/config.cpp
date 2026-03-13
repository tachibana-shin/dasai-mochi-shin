#include "config.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <SPIFFS.h>

#include "filesystem.h"

AppConfig config;

void AppConfig::fromJson(const JsonObject& doc) {
  brightness = doc["brightness"] | brightness;
  wifiEnabled = doc["wifiEnabled"] | wifiEnabled;
  bluetoothEnabled = doc["bluetoothEnabled"] | bluetoothEnabled;
  weatherInterval = doc["weatherInterval"] | weatherInterval;
  autoOffHour = doc["autoOffHour"] | autoOffHour;
  autoOnHour = doc["autoOnHour"] | autoOnHour;

  gmtOffset_sec = doc["gmtOffset_sec"] | gmtOffset_sec;
  daylightOffset_sec = doc["daylightOffset_sec"] | daylightOffset_sec;

  is24Hour = doc["is24Hour"] | is24Hour;
  langCode = doc["langCode"] | langCode;

  wifiAPName = doc["wifiAPName"] | wifiAPName;
  bluetoothName = doc["bluetoothName"] | bluetoothName;
  ntpServer = doc["ntpServer"] | ntpServer;

  weatherServer = doc["weatherServer"] | weatherServer;
  configPath = doc["configPath"] | configPath;
  mochiSpeedDivisor = doc["mochiSpeedDivisor"] | mochiSpeedDivisor;

  screenFlipMode = doc["screenFlipMode"] | screenFlipMode;
  screenNegative = doc["screenNegative"] | screenNegative;
  screenWidth = doc["screenWidth"] | screenWidth;
  screenHeight = doc["screenHeight"] | screenHeight;

  wifi.clear();
  if (doc["wifi"].is<JsonArray>()) {
    JsonArray wifiArray = doc["wifi"].as<JsonArray>();
    for (JsonObject w : wifiArray) {
      WifiEntry e;
      e.ssid = w["ssid"] | "";
      e.pass = w["pass"] | "";
      wifi.push_back(e);
    }
  }
}

void AppConfig::fromJsonBoot(const JsonObject& doc) {
  pinScreenSDA = doc["pinScreenSDA"] | pinScreenSDA;
  pinScreenSCL = doc["pinScreenSCL"] | pinScreenSCL;
  pinSensorTap = doc["pinSensorTap"] | pinSensorTap;

  pinSdCS = doc["pinSdCS"] | pinSdCS;
  pinSdMOSI = doc["pinSdMOSI"] | pinSdMOSI;
  pinSdCLK = doc["pinSdCLK"] | pinSdCLK;
  pinSdMISO = doc["pinSdMISO"] | pinSdMISO;
}

void AppConfig::toJson(JsonDocument& doc) const {
  doc["brightness"] = brightness;
  doc["wifiEnabled"] = wifiEnabled;
  doc["bluetoothEnabled"] = bluetoothEnabled;
  doc["weatherInterval"] = weatherInterval;
  doc["autoOffHour"] = autoOffHour;
  doc["autoOnHour"] = autoOnHour;

  doc["gmtOffset_sec"] = gmtOffset_sec;
  doc["daylightOffset_sec"] = daylightOffset_sec;

  doc["is24Hour"] = is24Hour;
  doc["langCode"] = langCode;

  doc["wifiAPName"] = wifiAPName;
  doc["bluetoothName"] = bluetoothName;
  doc["ntpServer"] = ntpServer;

  doc["weatherServer"] = weatherServer;
  doc["configPath"] = configPath;

  doc["screenFlipMode"] = screenFlipMode;
  doc["screenNegative"] = screenNegative;
  doc["screenWidth"] = screenWidth;
  doc["screenHeight"] = screenHeight;

  doc["mochiSpeedDivisor"] = mochiSpeedDivisor;

  JsonArray arr = doc["wifi"].to<JsonArray>();
  for (auto& e : wifi) {
    JsonObject w = arr.add<JsonObject>();
    w["ssid"] = e.ssid;
    w["pass"] = e.pass;
  }
}

void AppConfig::toJsonBoot(JsonDocument& doc) const {
  doc["pinScreenSDA"] = pinScreenSDA;
  doc["pinScreenSCL"] = pinScreenSCL;
  doc["pinSensorTap"] = pinSensorTap;

  doc["pinSdCS"] = pinSdCS;
  doc["pinSdMOSI"] = pinSdMOSI;
  doc["pinSdCLK"] = pinSdCLK;
  doc["pinSdMISO"] = pinSdMISO;
}

void AppConfig::debugPrint() const {
  Serial.println("Config loaded:");
  Serial.printf("  brightness: %d\n", brightness);
  Serial.printf("  wifiEnabled: %d\n", wifiEnabled);
  Serial.printf("  bluetoothEnabled: %d\n", bluetoothEnabled);
  Serial.printf("  weatherInterval: %lu\n", weatherInterval);
  Serial.printf("  autoOffHour: %d\n", autoOffHour);
  Serial.printf("  autoOnHour: %d\n", autoOnHour);

  Serial.printf("  gmtOffset_sec: %ld\n", gmtOffset_sec);
  Serial.printf("  daylightOffset_sec: %d\n", daylightOffset_sec);
  Serial.printf("  is24Hour: %d\n", is24Hour);
  Serial.printf("  langCode: %s\n", langCode.c_str());

  Serial.printf("  wifiAPName: %s\n", wifiAPName.c_str());
  Serial.printf("  bluetoothName: %s\n", bluetoothName.c_str());
  Serial.printf("  ntpServer: %s\n", ntpServer.c_str());
  Serial.printf("  weatherServer: %s\n", weatherServer.c_str());

  Serial.printf("  Screen SDA: %d, SCL: %d\n", pinScreenSDA, pinScreenSCL);
  Serial.printf("  Tap sensor: %d\n", pinSensorTap);
  Serial.printf("  SD pins: CS=%d MOSI=%d CLK=%d MISO=%d\n", pinSdCS, pinSdMOSI,
                pinSdCLK, pinSdMISO);

  Serial.printf("  wifi entries: %d\n", wifi.size());
  for (auto& e : wifi) {
    Serial.printf("    - ssid: %s, pass: %s\n", e.ssid.c_str(), e.pass.c_str());
  }
}

bool loadBootConfig() {
  fs::File configBootFile = getFile("/boot_config.json", FILE_READ, false);
  if (!configBootFile) {
    Serial.println("Failed to open boot_config.json");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, configBootFile);
  configBootFile.close();

  if (err) {
    Serial.println("Failed to parse boot_config.json");
    return false;
  }

  config.fromJsonBoot(doc.as<JsonObject>());

  Serial.println("Boot config loaded.");
  return true;
}

bool loadConfig() {
  fs::File configFile = getFile(config.configPath, FILE_READ);
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  config.fromJson(doc.as<JsonObject>());
  config.debugPrint();
  return true;
}

bool saveBootConfig() {
  JsonDocument doc;
  config.toJsonBoot(doc);

  fs::File configFile = getFile("/boot_config.json", FILE_WRITE, false);
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return false;
  }

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write config_boot file");
    configFile.close();
    return false;
  }

  configFile.close();
  Serial.println("boot_config.json saved successfully");
  return true;
}

bool saveConfig() {
  JsonDocument doc;
  config.toJson(doc);

  fs::File configFile = getFile(config.configPath, FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write config file");
    configFile.close();
    return false;
  }

  configFile.close();
  Serial.println("Config saved successfully");
  return true;
}
