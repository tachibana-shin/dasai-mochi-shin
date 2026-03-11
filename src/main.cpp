#include <Arduino.h>
#include <ArduinoJson.h>
#include <ChronosESP32.h>
#include <FS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <esp_wifi.h>
#include <time.h>
#include <vector>
#include <SD.h>
#include <SPI.h>

//////////////////////
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

  // ----------------------------------------------------------
  // Deserialize
  // ----------------------------------------------------------
  void fromJson(const JsonObject& doc) {
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

    // wifi entries
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

  // ----------------------------------------------------------
  // Serialize
  // ----------------------------------------------------------
  void toJson(JsonDocument& doc) const {
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

  // ----------------------------------------------------------
  // Debug log
  // ----------------------------------------------------------
  void debugPrint() const {
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
    Serial.printf("  SD pins: CS=%d MOSI=%d CLK=%d MISO=%d\n",
                  pinSdCS, pinSdMOSI, pinSdCLK, pinSdMISO);

    Serial.printf("  wifi entries: %d\n", wifi.size());
    for (auto& e : wifi) {
      Serial.printf("    - ssid: %s, pass: %s\n",
                    e.ssid.c_str(), e.pass.c_str());
    }
  }
};

AppConfig config;


struct LocaleInfo {
  const char* am;
  const char* pm;
  const char* months[12];
  const char* days[7];
};

const LocaleInfo locale_vi = {
  "SA", "CH",
  {
    "thg 1", "thg 2", "thg 3", "thg 4", "thg 5","thg 6",
    "thg 7", "thg 8", "thg 9", "thg 10", "thg 11", "thg 12"
  },
  {
    "CN", "T2", "T3", "T4", "T5", "T6", "T7"
  }
};
const LocaleInfo locale_en = {
  "AM", "PM",
  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  },
  {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  }
};

const LocaleInfo* getActiveLocale() {
  if (config.langCode == "vi") {
    return &locale_vi;
  }

  return &locale_en;
}

String onlineSunrise = "--:--";
String onlineSunset = "--:--";
volatile bool weatherUpdating = false;

//////////

#define OFFSET_DATE_X 2
#define OFFSET_DATE_Y 10
#define OFFSET_STATUSBAR_ICON_RIGHT_X 100
#define OFFSET_STATUSBAR_ICON_RIGHT_Y 10
#define OFFSET_TIME_X 2
#define OFFSET_TIME_Y 45
#define OFFSET_SEC_X 72
#define OFFSET_SEC_Y 35
#define OFFSET_WEATHER_ICON_X 90
#define OFFSET_WEATHER_ICON_Y 40
#define OFFSET_WEATHER_TEMP_X 90
#define OFFSET_WEATHER_TEMP_Y 55
#define OFFSET_HUM_X 110
#define OFFSET_HUM_Y 33
#define OFFSET_WIND_X 110
#define OFFSET_WIND_Y 42

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// Custom Button Handling
unsigned long lastTransitionTime = 0;
unsigned long lastReleaseTime = 0;
int buttonClicks = 0;
bool lastRawState = HIGH;      // Assuming INPUT_PULLUP (HIGH = released)
bool debouncedState = HIGH;
const unsigned long debounceDelay = 25;
const unsigned long multiClickDelay = 500;

Preferences preferences;
ChronosESP32 chronos;

int onlineTemp = -999;
int onlineHumidity = -1;
float onlineWindSpeed = -1.0;
int onlineWeatherCode = -1;
bool onlineIsDay = true;
unsigned long lastWeatherUpdate = 0;
unsigned long lastChronoUpdate = 0;
bool isPortalActive = false;
bool screenOn = true;

unsigned long lastScreenAutoCheck = 0;
const unsigned long screenAutoCheckInterval = 60000;

SPIClass sdSPI(HSPI);

bool initSD() {
  sdSPI.begin(config.pinSdCLK, config.pinSdMISO, config.pinSdMOSI, config.pinSdCS);
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

  JsonArray wifiArray = doc["wifi"].to<JsonArray>();
  for (auto &w : config.wifi) {
    JsonObject obj = wifiArray.add<JsonObject>();
    obj["ssid"] = w.ssid;
    obj["pass"] = w.pass;
  }

  File configFile;
  bool useSD = initSD();

  if (useSD) {
    configFile = SD.open(config.configPath, FILE_WRITE);
    if (!configFile) {
      Serial.println("Failed to open config file on SD for writing");
      // fallback to SPIFFS
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

void toggleScreen() {
  screenOn = !screenOn;
  u8g2.setPowerSave(!screenOn);
  Serial.println(screenOn ? "Screen ON" : "Screen OFF");
}

void checkScreenAutoOff() {
  // Check interval
  if (millis() - lastScreenAutoCheck < screenAutoCheckInterval) return;
  lastScreenAutoCheck = millis();

  int onHour = config.autoOnHour;
  int offHour = config.autoOffHour;

  // If both are -1, feature disabled
  if (onHour == -1 && offHour == -1) return;

  int currentHour = -1;
  struct tm timeinfo;

  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
    currentHour = timeinfo.tm_hour;
  } else {
    // Assume chronos.getHour() returns 0-23
    currentHour = chronos.getHour();
  }

  if (currentHour < 0 || currentHour > 23) return; // time not available

  bool shouldBeOn;

  if (onHour != -1 && offHour != -1) {
    // Both set: use interval logic (could wrap)
    if (onHour <= offHour) {
      shouldBeOn = (currentHour >= onHour && currentHour < offHour);
    } else {
      shouldBeOn = (currentHour >= onHour || currentHour < offHour);
    }
  } else if (onHour != -1) {
    // Only onHour set: screen on from onHour onward (onHour to 23)
    shouldBeOn = (currentHour >= onHour);
  } else { // only offHour set
    // Only offHour set: screen on from 0 to offHour-1
    shouldBeOn = (currentHour < offHour);
  }

  if (shouldBeOn && !screenOn) {
    toggleScreen();
  } else if (!shouldBeOn && screenOn) {
    toggleScreen();
  }
}

bool openWiFiManager() {
  isPortalActive = true;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2.drawStr(0, 20, "WiFi Setup");
  u8g2.sendBuffer();

  WiFiManager wm;
  wm.setClass("invert");
  wm.setTimeout(180);
  wm.setConfigPortalTimeout(180);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  u8g2.setCursor(0, 40);
  u8g2.print("AP: ");
  u8g2.print(config.wifiAPName);
  u8g2.sendBuffer();

  bool ok = wm.autoConnect(config.wifiAPName.c_str());
  isPortalActive = false;

  if (!ok) {
    Serial.println("[WiFiManager] Failed or timeout");
    return false;
  }

  WifiEntry e;
  e.ssid = WiFi.SSID();
  e.pass = WiFi.psk();
  config.wifi.push_back(e);
  saveConfig();

  Serial.println("[WiFiManager] New WiFi Saved");
  return true;
}
static bool wifiConnectNew(String ssid, String pass, bool connect) {
  WiFi.persistent(true);
  WiFi.enableSTA(true);
  WiFi.persistent(false);

  WiFi.persistent(true);
  bool ret = WiFi.begin(ssid.c_str(), pass.c_str(), 0, NULL, connect);
  WiFi.persistent(false);
  return ret;
}

static void connectWiFi() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2.drawStr(0, 20, "WiFi...");
  u8g2.sendBuffer();

  Serial.println("[WiFi] Attempting connections from config...");

  for (auto &w : config.wifi) {
    Serial.println("[WiFi] Trying: " + w.ssid);
    if (wifiConnectNew(w.ssid, w.pass, true)) {
      int retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500);
        retry++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected by saved list!");
        return;
      }
    }
  }

  Serial.println("[WiFi] All saved failed, opening WiFiManager");
  if (!openWiFiManager()) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "WiFi Failed");
    u8g2.sendBuffer();
    delay(1500);
    return;
  }

  Serial.println("[WiFi] Reconnecting with new credentials...");
  wifiConnectNew(WiFi.SSID(), WiFi.psk(), true);
}

void handleClick() {
  Serial.println("Clicked!");
  config.brightness += 50;
  if (config.brightness > 255) config.brightness = 255;
  u8g2.setContrast(config.brightness);
  saveConfig();
}

void handleDoubleclick() {
  Serial.println("Double clicked!");
  config.brightness -= 50;
  if (config.brightness < 5) config.brightness = 5;
  u8g2.setContrast(config.brightness);
  saveConfig();
}

void handleTripleClick() {
  Serial.println("Triple clicked!");
  config.wifiEnabled = !config.wifiEnabled;
  saveConfig();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  if (config.wifiEnabled) {
    u8g2.drawStr(0, 20, "WiFi ON");
    u8g2.sendBuffer();
    delay(1000);
    connectWiFi();
  } else {
    u8g2.drawStr(0, 20, "WiFi OFF");
    u8g2.sendBuffer();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(1000);
  }
}

void checkButton() {
  bool rawState = digitalRead(config.pinSensorTap);

  if (rawState != lastRawState) {
    lastTransitionTime = millis();
  }
  lastRawState = rawState;

  if ((millis() - lastTransitionTime) > debounceDelay) {
    if (rawState != debouncedState) {
      debouncedState = rawState;
      if (debouncedState == HIGH) {
        if (!screenOn) {
          toggleScreen();
          buttonClicks = 0;
        } else {
          buttonClicks++;
          lastReleaseTime = millis();
        }
      }
    }
  }

  if (buttonClicks > 0 && (millis() - lastReleaseTime > multiClickDelay)) {
    if (buttonClicks == 1) handleClick();
    else if (buttonClicks == 2) handleDoubleclick();
    else if (buttonClicks == 3) handleTripleClick();
    else if (buttonClicks >= 4) toggleScreen();
    buttonClicks = 0;
  }
}

void weatherTask(void *pvParameters) {
  weatherUpdating = true;
  Serial.println("[Weather] Background update started...");

  HTTPClient http;
  http.begin(config.weatherServer);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      onlineTemp = doc["current"]["temperature_2m"].as<int>();
      onlineHumidity = doc["current"]["relative_humidity_2m"].as<int>();
      onlineWindSpeed = doc["current"]["wind_speed_10m"].as<float>();
      onlineWeatherCode = doc["current"]["weather_code"].as<int>();
      onlineIsDay = doc["current"]["is_day"].as<int>() == 1;

      String sr = doc["daily"]["sunrise"][0].as<String>();
      String ss = doc["daily"]["sunset"][0].as<String>();
      if (sr.length() > 11) onlineSunrise = sr.substring(11);
      if (ss.length() > 11) onlineSunset = ss.substring(11);

      lastWeatherUpdate = millis();
      Serial.printf("[Weather] Success: %dC\n", onlineTemp);
    } else {
      Serial.println("[Weather] JSON parse failed");
    }
  } else {
    Serial.println("[Weather] HTTP failed: " + String(httpCode));
  }
  http.end();

  weatherUpdating = false;
  vTaskDelete(NULL); // 終了時にタスクを自動削除
}

bool updateOnlineWeather() {
  if (weatherUpdating || WiFi.status() != WL_CONNECTED)
    return false;

  // コア1でタスクを実行（コア0は通常WiFi用）
  xTaskCreatePinnedToCore(
      weatherTask,      // 実行関数
      "weatherTask",    // タスク名
      8192,             // スタックサイズ
      NULL,             // パラメータ
      1,                // 優先度
      NULL,             // タスクハンドル
      1                 // コア1で実行
  );

  return true;
}

bool syncNTP() {
  if (WiFi.status() == WL_CONNECTED) {
    configTime(config.gmtOffset_sec, config.daylightOffset_sec, config.ntpServer.c_str());
    Serial.println("NTP sync started");
    return true;
  }
  return false;
}
static void syncLocalFromChronos() {
  struct tm t;
  memset(&t, 0, sizeof(t));

  t.tm_year = chronos.getYear() - 1900;
  t.tm_mon  = chronos.getMonth() - 1;
  t.tm_mday = chronos.getDay();
  t.tm_hour = chronos.getHour();
  t.tm_min  = chronos.getMinute();
  t.tm_sec  = chronos.getSecond();

  time_t now = mktime(&t);

  struct timeval tv;
  tv.tv_sec = now;
  tv.tv_usec = 0;

  settimeofday(&tv, nullptr);
}

void drawStatusBar() {
  u8g2.setFont(u8g2_font_6x10_tf); // Switch to unifont for multi-language support

  // Left: Date
  String dateStr;
  const LocaleInfo* loc = getActiveLocale();
  struct tm timeinfo;

  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
    char buffer[48];
    snprintf(buffer, sizeof(buffer), "%s, %02d%s", loc->days[timeinfo.tm_wday], timeinfo.tm_mday, loc->months[timeinfo.tm_mon]);
    
    dateStr = String(buffer);
  } else {
    dateStr = chronos.getTime("%d %b");
  }
  u8g2.drawStr(OFFSET_DATE_X, OFFSET_DATE_Y, dateStr.c_str());

  // Bluetooth Icon
  u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
  u8g2.drawGlyph(OFFSET_STATUSBAR_ICON_RIGHT_X, OFFSET_STATUSBAR_ICON_RIGHT_Y, 74);
  if (!config.bluetoothEnabled || !chronos.isConnected()) {
    u8g2.drawLine(OFFSET_STATUSBAR_ICON_RIGHT_X, OFFSET_STATUSBAR_ICON_RIGHT_Y, OFFSET_STATUSBAR_ICON_RIGHT_X + 7, OFFSET_STATUSBAR_ICON_RIGHT_Y - 7);
  }

  // Wifi Icon
  u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
  u8g2.drawGlyph(OFFSET_STATUSBAR_ICON_RIGHT_X + 10, OFFSET_STATUSBAR_ICON_RIGHT_Y, 0x0051);
  if (!config.wifiEnabled || WiFi.status() != WL_CONNECTED) {
    u8g2.drawLine(OFFSET_STATUSBAR_ICON_RIGHT_X + 10 + 1, OFFSET_STATUSBAR_ICON_RIGHT_Y, OFFSET_STATUSBAR_ICON_RIGHT_X + 12 + 7, OFFSET_STATUSBAR_ICON_RIGHT_Y - 7);
  }
}

void drawMainClock() {
  // Left: Clock
  u8g2.setFont(u8g2_font_logisoso24_tn);
  String hourStr, minuteStr, secStr, ampmStr;

  struct tm timeinfo;
  bool timeValid = false;
  if (getLocalTime(&timeinfo)) {
    timeValid = true;
    char h[3], m[3], s[3], p[3];
    if (config.is24Hour) {
      strftime(h, sizeof(h), "%H", &timeinfo);   // 00–23
      ampmStr = "";                           
    } else {
      strftime(h, sizeof(h), "%I", &timeinfo);   // 01–12
      strftime(p, sizeof(p), "%p", &timeinfo);   // AM / PM
      ampmStr = String(p);
    }

    // 分と秒
    strftime(m, sizeof(m), "%M", &timeinfo);
    strftime(s, sizeof(s), "%S", &timeinfo);

    hourStr = String(h);
    minuteStr = String(m);
    secStr = String(s);
  }

  const LocaleInfo* loc = getActiveLocale();
  if (ampmStr == "AM") ampmStr = loc->am;
  else if (ampmStr == "PM") ampmStr = loc->pm;

  String timeStr = hourStr + ":" + minuteStr;
  u8g2.drawStr(OFFSET_TIME_X, OFFSET_TIME_Y, timeStr.c_str()); // Corrected from OFFSET_Y_TIME

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(OFFSET_SEC_X, OFFSET_SEC_Y, secStr.c_str()); // Seconds above AM/PM
  u8g2.drawStr(OFFSET_SEC_X, OFFSET_SEC_Y + 10, ampmStr.c_str());

  // Right: Weather Icon
  uint8_t iconGlyph = 0x41; // デフォルト：曇り（不明）
  int weatherCode = -1;
  bool isDay = true;

  if (WiFi.status() == WL_CONNECTED && onlineWeatherCode != -1) {
    weatherCode = onlineWeatherCode;
    isDay = onlineIsDay;
    
    // WMOコードをアイコンにマッピング
    if (weatherCode == 0) iconGlyph = isDay ? 0x45 : 0x44; // 太陽 / 月
    else if (weatherCode == 1 || weatherCode == 2) iconGlyph = 0x46; // 雲 + 太陽
    else if (weatherCode == 3 || weatherCode == 45 || weatherCode == 48) iconGlyph = 0x41; // 雲
    else if ((weatherCode >= 51 && weatherCode <= 65) || (weatherCode >= 80 && weatherCode <= 82)) iconGlyph = 0x43; // 雨
    else if ((weatherCode >= 71 && weatherCode <= 77) || (weatherCode >= 85 && weatherCode <= 86)) iconGlyph = 0x42; // 雪 / 傘
    else if (weatherCode >= 95) iconGlyph = 0x47; // 雷
  }

  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
  u8g2.drawGlyph(OFFSET_WEATHER_ICON_X, OFFSET_WEATHER_ICON_Y, iconGlyph);

  u8g2.setFont(u8g2_font_ncenB10_tr);
  String tempStr = "??";
  int humidity = -1;
  float wind = -1;

  if (WiFi.status() == WL_CONNECTED && onlineTemp != -999) {
    tempStr = String(onlineTemp);
    humidity = onlineHumidity;
    wind = onlineWindSpeed;
  } else if (chronos.getWeatherCount() > 0) {
    tempStr = String(chronos.getWeatherAt(0).temp);
  }

  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  if (tempStr != "??") tempStr += "C";
  u8g2.drawStr(OFFSET_WEATHER_TEMP_X, OFFSET_WEATHER_TEMP_Y, tempStr.c_str());

  // Sunrise / Sunset at the bottom
  if (onlineSunrise != "" && onlineSunset != "") {
    u8g2.setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2.drawGlyph(2, 63, 0x45); // 日の出：太陽アイコン
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(12, 63, onlineSunrise.c_str());

    u8g2.setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2.drawGlyph(65, 63, 0x44); // 日の入り：月アイコン
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(75, 63, onlineSunset.c_str());
  }

  // Humidity and Wind (Small)
  u8g2.setFont(u8g2_font_5x7_tf);
  String humStr = "H";
  String windStr = "W";
  
  if (humidity != -1) humStr += String(humidity) + "%";
  else humStr += "??";
  
  if (wind != -1) windStr += String(wind, 1) + "m/s";
  else windStr += "??";

  u8g2.drawStr(OFFSET_HUM_X, OFFSET_HUM_Y, humStr.c_str());
  u8g2.drawStr(OFFSET_WIND_X, OFFSET_WIND_Y, windStr.c_str());
}

// ---------- Setup ----------
void setup(void) {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_MODE_APSTA);
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  wifi_config_t cfg;
  esp_wifi_get_config(WIFI_IF_AP, &cfg);
  cfg.ap.channel = 1;
  cfg.ap.max_connection = 4;
  esp_wifi_set_config(WIFI_IF_AP, &cfg);

  loadConfig();
  Wire.begin(config.pinScreenSDA, config.pinScreenSCL);

  u8g2.begin();
  u8g2.setContrast(config.brightness);
  u8g2.enableUTF8Print();

  pinMode(config.pinSensorTap, INPUT_PULLUP);

  chronos.setName(config.bluetoothName);

  chronos.setConnectionCallback([](bool enabled) {
    if (enabled) syncLocalFromChronos();
  });
  chronos.setNotificationCallback([] (Notification notify) {
    // notification
  });
  chronos.setConfigurationCallback([](Config _, uint32_t timestamp, uint32_t timezone) {
syncLocalFromChronos();

  bool is24Hour =chronos.is24Hour();
  if (is24Hour != config.is24Hour) {
    config.is24Hour = is24Hour;
    saveConfig();
  }
  });

  if (config.bluetoothEnabled) {
  chronos.begin();
  }
  if (config.wifiEnabled) {
    connectWiFi();
  }
}

void loop(void) {
  checkButton();
    if (config.bluetoothEnabled) {
    chronos.loop();
    }

  if (isPortalActive) {
    // WiFiManager is blocking
  } else {

    if (config.wifiEnabled && WiFi.status() == WL_CONNECTED && !weatherUpdating) {
      if (millis() - lastWeatherUpdate > config.weatherInterval || lastWeatherUpdate == 0) {
        syncNTP();
        updateOnlineWeather();
      }
    }

    checkScreenAutoOff();

    u8g2.clearBuffer();
    drawStatusBar();
    drawMainClock();
    u8g2.sendBuffer();
  }

  delay(10);
}
