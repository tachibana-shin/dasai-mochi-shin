#include "web_server.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <SD.h>

#include "config.h"
#include "display.h"
#include "filesystem.h"
#include "audio_player.h"
#include "time_utils.h"

AsyncWebServer server(80);

static bool sMdnsStarted = false;
void ensureMdnsStarted();

static bool isPinField(const String& key) {
  return key.startsWith("pin");
}

String sanitizeMdnsName(String name) {
  name.toLowerCase();
  name.replace(" ", "-");
  String result = "";
  for (char c : name) {
    if (isAlphaNumeric(c) || c == '-') {
      result += c;
    }
  }
  return result;
}

void setupApiEndpoints() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) { request->send(200); return; }
    String path = request->url();
    if (path.startsWith("/api/")) { request->send(404); return; }
    if (path == "/" || path.isEmpty()) path = "/index.html";

    String ct = "text/html";
    if (path.endsWith(".js")) ct = "application/javascript";
    else if (path.endsWith(".css")) ct = "text/css";
    else if (path.endsWith(".woff2")) ct = "font/woff2";
    else if (path.endsWith(".woff")) ct = "font/woff";
    else if (path.endsWith(".ttf")) ct = "font/ttf";
    else if (path.endsWith(".eot")) ct = "application/vnd.ms-fontobject";
    else if (path.endsWith(".svg")) ct = "image/svg+xml";
    else if (path.endsWith(".json")) ct = "application/json";
    else if (path.endsWith(".png")) ct = "image/png";

    auto serveFile = [&](fs::File f, size_t sz, const String& enc) {
      Serial.printf("[serve] %s (%u)%s\n", path.c_str(), sz, enc.length() ? " gzip" : "");
      AsyncWebServerResponse *rsp = request->beginChunkedResponse(
        ct, [f, sz](uint8_t* buf, size_t maxLen, size_t index) mutable -> size_t {
          if (maxLen == 0 || index >= sz || !f) {
            if (f) f.close();
            return 0;
          }
          f.seek(index);
          size_t n = f.read(buf, min(maxLen, sz - index));
          if (n == 0) f.close();
          return n;
        });
      if (enc.length()) rsp->addHeader("Content-Encoding", enc);
      rsp->setCode(200);
      request->send(rsp);
    };

    bool acceptGzip = request->header("Accept-Encoding").indexOf("gzip") >= 0;
    fs::File sf;

    if (acceptGzip) sf = LittleFS.open(path + ".gz", "r");
    if (sf && sf.size() > 0) { serveFile(sf, sf.size(), "gzip"); return; }
    if (sf) sf.close();

    sf = LittleFS.open(path, "r");
    if ((!sf || sf.size() == 0) && sdInitialized) sf = SD.open(path, "r");
    if (sf && sf.size() > 0) { serveFile(sf, sf.size(), ""); return; }
    if (sf) sf.close();

    ct = "text/html";
    sf = LittleFS.open("/index.html", "r");
    if (sf && sf.size() > 0) { serveFile(sf, sf.size(), ""); return; }
    if (sf) sf.close();
    request->send(404, "text/plain", "Not Found");
  });

  // GET /api/info
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["module"] = config.wifiAPName;
    doc["ip"] = WiFi.localIP().toString();
    doc["cpu"] = "ESP32-C3 @ 80MHz";
    doc["ram_free"] = ESP.getFreeHeap() / 1024;
    doc["ram_total"] = ESP.getHeapSize() / 1024;
    doc["flash_free"] = (LittleFS.totalBytes() - LittleFS.usedBytes()) / 1024;
    doc["flash_total"] = LittleFS.totalBytes() / 1024;

    bool sdOk = false;
    if (SD.cardSize() > 0) {
      sdOk = true;
      doc["sd_total"] = (uint32_t)(SD.totalBytes() / 1024);
      doc["sd_free"] = (uint32_t)((SD.totalBytes() - SD.usedBytes()) / 1024);
    }
    doc["sd_ok"] = sdOk;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // GET /api/config
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    config.toJson(doc);
    config.toJsonBoot(doc);
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // GET /api/wifi/scan
  server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
      WiFi.scanNetworks(true);
      request->send(202, "application/json", "{\"status\":\"scanning\"}");
    } else if (n == WIFI_SCAN_RUNNING) {
      request->send(202, "application/json", "{\"status\":\"scanning\"}");
    } else {
      JsonDocument doc;
      JsonArray arr = doc.to<JsonArray>();
      for (int i = 0; i < n; i++) {
        JsonObject net = arr.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["encryption"] = (int)WiFi.encryptionType(i);
      }
      WiFi.scanDelete();
      String output;
      serializeJson(doc, output);
      request->send(200, "application/json", output);
    }
  });

  // POST /api/update (Partial update with real-time reactions)
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/api/update", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonDocument resp;
    JsonObject obj = json.as<JsonObject>();
    config.fromJson(obj);
    config.fromJsonBoot(obj);

    bool pinChanged = false;
    for (JsonPair kv : obj) {
      if (isPinField(kv.key().c_str())) {
        pinChanged = true;
        break;
      }
    }
    if (pinChanged) saveBootConfig();

    if (obj.containsKey("brightness")) {
      applyDisplayConfig();
    }
    if (obj.containsKey("screenFlipMode") || obj.containsKey("screenNegative")) {
      applyDisplayConfig();
    }
    if (obj.containsKey("wifiEnabled")) {
      if (config.wifiEnabled && WiFi.status() != WL_CONNECTED) {
        WiFi.begin();
      } else if (!config.wifiEnabled) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      }
    }
    if (obj.containsKey("gmtOffset_sec") || obj.containsKey("ntpServer") || obj.containsKey("daylightOffset_sec")) {
      syncNTP();
    }
    if (obj.containsKey("audio")) {
      audioInit();
    }

    if (saveConfig()) {
      if (pinChanged) {
        resp["status"] = "ok";
        resp["reboot_required"] = true;
        resp["message"] = "Pin changes require reboot to take effect";
      } else {
        resp["status"] = "ok";
      }
    } else {
      resp["status"] = "error";
      resp["message"] = "Failed to save config";
      request->send(500, "application/json", resp.as<String>());
      return;
    }
    String output;
    serializeJson(resp, output);
    request->send(200, "application/json", output);
  });
  server.addHandler(handler);

  // POST /api/config (Full update)
  AsyncCallbackJsonWebHandler *fullConfigHandler = new AsyncCallbackJsonWebHandler("/api/config", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonDocument resp;
    JsonObject obj = json.as<JsonObject>();
    config.fromJson(obj);
    config.fromJsonBoot(obj);

    bool pinChanged = false;
    for (JsonPair kv : obj) {
      if (isPinField(kv.key().c_str())) {
        pinChanged = true;
        break;
      }
    }
    if (pinChanged) saveBootConfig();

    applyDisplayConfig();
    audioInit();

    if (config.wifiEnabled && WiFi.status() != WL_CONNECTED) {
      WiFi.begin();
    } else if (!config.wifiEnabled) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
    syncNTP();

    if (saveConfig()) {
      if (pinChanged) {
        resp["status"] = "ok";
        resp["reboot_required"] = true;
        resp["message"] = "Pin changes require reboot to take effect";
      } else {
        resp["status"] = "ok";
      }
    } else {
      resp["status"] = "error";
      resp["message"] = "Failed to save config";
      request->send(500, "application/json", resp.as<String>());
      return;
    }
    String output;
    serializeJson(resp, output);
    request->send(200, "application/json", output);
  });
  server.addHandler(fullConfigHandler);

  // POST /api/time/sync
  server.on("/api/time/sync", HTTP_POST, [](AsyncWebServerRequest *request) {
    syncNTP();
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // POST /api/preview/audio
  server.on("/api/preview/audio", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("path", true)) {
      String path = request->getParam("path", true)->value();
      fs::File file = getFile(config.homePath + "/" + path, FILE_READ);
      if (file) {
        audioPlayFile(file);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"File not found\"}");
      }
    } else {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing path parameter\"}");
    }
  });

  // GET /api/files/list
  server.on("/api/files/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    String dir = "/Mochi";
    if (request->hasParam("dir")) {
      dir = request->getParam("dir")->value();
    }
    String fullPath = config.homePath + dir;
    Serial.printf("[files/list] path=%s sdInit=%d\n", fullPath.c_str(), sdInitialized);

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    File root;

    if (sdInitialized) {
      root = SD.open(fullPath);
      Serial.printf("[files/list] SD.open=%s\n", root ? "OK" : "FAIL");
    }
    if (!root) {
      root = LittleFS.open(fullPath);
      Serial.printf("[files/list] LittleFS.open=%s\n", root ? "OK" : "FAIL");
    }
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        if (!file.isDirectory()) {
          JsonObject fileObj = arr.add<JsonObject>();
          fileObj["name"] = String(file.name());
          fileObj["size"] = file.size();
          Serial.printf("[files/list] found: %s (%u)\n", file.name(), file.size());
        }
        file.close();
        file = root.openNextFile();
      }
      root.close();
    } else if (root) {
      Serial.printf("[files/list] root is NOT a directory\n");
      root.close();
    }
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // DELETE /api/files/delete
  server.on("/api/files/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (request->hasParam("path")) {
      String path = request->getParam("path")->value();
      bool ok = LittleFS.remove(path) || SD.remove(path);
      if (ok) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to delete file\"}");
      }
    } else {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing path parameter\"}");
    }
  });

  // GET /api/debug/fs
  server.on("/api/debug/fs", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    std::vector<String> dirs = {"/"};
    while (!dirs.empty()) {
      String d = dirs.back(); dirs.pop_back();
      File root = LittleFS.open(d);
      if (!root || !root.isDirectory()) continue;
      File f = root.openNextFile();
      while (f) {
        JsonObject o = arr.add<JsonObject>();
        o["name"] = String(f.name());
        o["size"] = f.size();
        o["isDir"] = f.isDirectory();
        if (f.isDirectory()) dirs.push_back(f.name());
        f.close();
        f = root.openNextFile();
      }
      root.close();
    }
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // POST /api/reboot
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(500);
    ESP.restart();
  });

  // File Upload (multipart/form-data)
  server.on("/api/files/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;
    if (!index) {
      String path = config.homePath + "/Mochi/" + filename;
      Serial.printf("Upload Start: %s\n", path.c_str());
      uploadFile = LittleFS.open(path, FILE_WRITE);
    }
    if (uploadFile) {
      uploadFile.write(data, len);
    }
    if (final) {
      if (uploadFile) {
        uploadFile.close();
      }
      Serial.printf("Upload End: %s\n", filename.c_str());
    }
  });
}

void initWebServer() {
  ensureMdnsStarted();
  setupApiEndpoints();
  server.begin();
  Serial.println("HTTP server started");
}

void ensureMdnsStarted() {
  if (sMdnsStarted || WiFi.status() != WL_CONNECTED) return;

  String mdnsName = sanitizeMdnsName(config.wifiAPName);
  if (mdnsName.length() == 0) mdnsName = "mochishin";

  if (MDNS.begin(mdnsName.c_str())) {
    sMdnsStarted = true;
    Serial.printf("mDNS responder started at http://%s.local\n", mdnsName.c_str());
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "name", config.wifiAPName);
    MDNS.addServiceTxt("http", "tcp", "model", "MochiShin");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }
}
