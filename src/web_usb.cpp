#include "web_usb.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPIFFS.h>

#include "alarm.h"
#include "audio_player.h"


#include "config.h"
#include "display.h"
#include "filesystem.h"
#include "reminder.h"

void initWebUsb() {}

void loopWebUsb() {
  static String lineBuffer = "";

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;  // ignore carriage return
    if (c != '\n') {
      lineBuffer += c;
      continue;
    }

    // Got a complete line
    String line = lineBuffer;
    lineBuffer = "";
    line.trim();
    if (line.length() == 0) continue;

    if (line == "GET_CONFIG") {
      JsonDocument doc;
      config.toJson(doc);
      config.toJsonBoot(doc);
      String output;
      serializeJson(doc, output);
      Serial.println("CONF:" + output);
    } else if (line == "GET_SYS_INFO") {
      JsonDocument doc;
      doc["module"] = config.wifiAPName;
      doc["cpu"] = "ESP32-C3 @ 80MHz";
      doc["ram_free"] = ESP.getFreeHeap() / 1024;
      doc["ram_total"] = ESP.getHeapSize() / 1024;

      // Internal Flash (SPIFFS)
      doc["flash_free"] = (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024;
      doc["flash_total"] = SPIFFS.totalBytes() / 1024;

      // SD Card Info
      bool sdOk = false;
      if (SD.cardSize() > 0) {
        sdOk = true;
        doc["sd_total"] = (uint32_t)(SD.totalBytes() / 1024);
        doc["sd_free"] = (uint32_t)((SD.totalBytes() - SD.usedBytes()) / 1024);
      }
      doc["sd_ok"] = sdOk;

      String output;
      serializeJson(doc, output);
      Serial.println("SYS:" + output);
    } else if (line == "LIST_MOCHI") {
      JsonDocument doc;
      JsonArray arr = doc.to<JsonArray>();
      File root = SPIFFS.open(config.homePath + "/Mochi");
      File file = root.openNextFile();
      while (file) {
        if (!file.isDirectory()) arr.add(String(file.name()));
        file = root.openNextFile();
      }
      String output;
      serializeJson(doc, output);
      Serial.println("FILES:" + output);
    } else if (line.startsWith("DELETE_FILE:")) {
      String path = line.substring(12);
      if (SPIFFS.remove(path))
        Serial.println("STATUS:OK");
      else
        Serial.println("STATUS:ERROR_DELETE");
    } else if (line.startsWith("SET_CONFIG:")) {
      String json = line.substring(11);
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, json);
      if (!error) {
        config.fromJson(doc.as<JsonObject>());
        config.fromJsonBoot(doc.as<JsonObject>());

        initDisplay();
        initReminder();
        initAlarm();
        audioInit();

        if (saveConfig())
          Serial.println("STATUS:OK");
        else
          Serial.println("STATUS:ERROR_SAVE");
      } else {
        Serial.print("STATUS:ERROR_JSON_");
        Serial.println(error.c_str());
      }
    } else if (line == "REBOOT") {
      Serial.println("STATUS:REBOOTING");
      delay(500);
      ESP.restart();
    }
  }
}