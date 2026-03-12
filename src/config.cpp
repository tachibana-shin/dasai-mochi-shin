#include "config.h"
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <SPI.h>

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

    pinScreenSDA = doc["pinScreenSDA"] | pinScreenSDA;
    pinScreenSCL = doc["pinScreenSCL"] | pinScreenSCL;
    pinSensorTap = doc["pinSensorTap"] | pinSensorTap;

    pinSdCS = doc["pinSdCS"] | pinSdCS;
    pinSdMOSI = doc["pinSdMOSI"] | pinSdMOSI;
    pinSdCLK = doc["pinSdCLK"] | pinSdCLK;
    pinSdMISO = doc["pinSdMISO"] | pinSdMISO;

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

    doc["pinScreenSDA"] = pinScreenSDA;
    doc["pinScreenSCL"] = pinScreenSCL;
    doc["pinSensorTap"] = pinSensorTap;

    doc["pinSdCS"] = pinSdCS;
    doc["pinSdMOSI"] = pinSdMOSI;
    doc["pinSdCLK"] = pinSdCLK;
    doc["pinSdMISO"] = pinSdMISO;

    JsonArray arr = doc["wifi"].to<JsonArray>();
    for (auto& e : wifi) {
        JsonObject w = arr.add<JsonObject>();
        w["ssid"] = e.ssid;
        w["pass"] = e.pass;
    }
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
    Serial.printf("  SD pins: CS=%d MOSI=%d CLK=%d MISO=%d\n", pinSdCS,
                  pinSdMOSI, pinSdCLK, pinSdMISO);

    Serial.printf("  wifi entries: %d\n", wifi.size());
    for (auto& e : wifi) {
        Serial.printf("    - ssid: %s, pass: %s\n", e.ssid.c_str(),
                      e.pass.c_str());
    }
}

static SPIClass sdSPI(HSPI);

static bool initSD() {
    sdSPI.begin(config.pinSdCLK, config.pinSdMISO, config.pinSdMOSI,
                config.pinSdCS);
    if (!SD.begin(config.pinSdCS, sdSPI)) {
        Serial.println("SD card initialization failed!");
        return false;
    }
    Serial.println("SD card initialized.");
    return true;
}

bool loadConfig() {
    File configFile;
    bool useSD = initSD();

    if (useSD && SD.exists(config.configPath)) {
        configFile = SD.open(config.configPath, FILE_READ);
        Serial.println("Loading config from SD");
    } else {
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS mount failed");
            return false;
        }
        if (SPIFFS.exists(config.configPath)) {
            configFile = SPIFFS.open(config.configPath, FILE_READ);
            Serial.println("Loading config from SPIFFS");
        } else {
            Serial.println("No config file found, using defaults");
            return false;
        }
    }

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

bool saveConfig() {
    JsonDocument doc;
    config.toJson(doc);

    File configFile;
    bool useSD = initSD();

    if (useSD) {
        configFile = SD.open(config.configPath, FILE_WRITE);
        if (!configFile) {
            Serial.println("Failed to open config file on SD for writing");
            useSD = false;
        }
    }

    if (!useSD) {
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS mount failed");
            return false;
        }
        configFile = SPIFFS.open(config.configPath, FILE_WRITE);
    }

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