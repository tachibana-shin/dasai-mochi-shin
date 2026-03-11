#include <Arduino.h>
#include <ArduinoJson.h>
#include <ChronosESP32.h>
#include <FS.h>
#include <HTTPClient.h>
#include <OneButton.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <esp_wifi.h>
#include <time.h>
#include <vector>

// allow edit
#define PIN_SDA 20
#define PIN_SCL 21
#define PIN_TAP 10

#define WIFI_AP_NAME "Dasai Mochi Shin"

const unsigned long weatherInterval = 1800000; // 30 mins
const char *ntpServer = "time.google.com";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

#define LANG_CODE "vi" // "vi" or "ja"

struct LocaleInfo {
  const char* am;
  const char* pm;
  const char* months[12];
  const char* days[7];
};

const LocaleInfo locale_vi = {
  "SA", "CH",
  {"thg 1", "thg 2", "thg 3", "thg 4", "thg 5", "thg 6", "thg 7", "thg 8", "thg 9", "thg 10", "thg 11", "thg 12"},
  {"CN", "T2", "T3", "T4", "T5", "T6", "T7"},
};

const LocaleInfo* getActiveLocale() {
  return &locale_vi;
}

#define WEATHER_SERVER "http://api.open-meteo.com/v1/forecast?latitude=10.762622&longitude=106.660172&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code,is_day&daily=sunrise,sunset&timezone=auto"

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
OneButton button;
Preferences preferences;
ChronosESP32 chronos("Mochi Shin");

int brightness = 150;
int onlineTemp = -999;
int onlineHumidity = -1;
float onlineWindSpeed = -1.0;
int onlineWeatherCode = -1;
bool onlineIsDay = true;
unsigned long lastWeatherUpdate = 0;
bool wifiEnabled = true;
bool isPortalActive = false;

struct WifiEntry {
  String ssid;
  String pass;
};

std::vector<WifiEntry> wifiList;

void saveBrightness() {
  preferences.begin("settings", false);  
  preferences.putInt("light", brightness); 
  preferences.end();
}
void saveWifiList() {
  JsonDocument doc;
  JsonArray arr = doc["wifi"].to<JsonArray>();

  for (auto &w : wifiList) {
    JsonObject obj = arr.add<JsonObject>();
    obj["ssid"] = w.ssid;
    obj["pass"] = w.pass;
  }

  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS failed");
    return;
  }

  File f = SPIFFS.open("/wifi_list.json", "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
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
  u8g2.print(WIFI_AP_NAME);
  u8g2.sendBuffer();

  bool ok = wm.autoConnect(WIFI_AP_NAME);
  isPortalActive = false;
  
  if (!ok) {
    Serial.println("[WiFiManager] Failed or timeout");
    return false;
  }

  WifiEntry e;
  e.ssid = WiFi.SSID();
  e.pass = WiFi.psk();
  wifiList.push_back(e);
  saveWifiList();

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

void loadWifiList() {
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS failed");
    return;
  }

  if (!SPIFFS.exists("/wifi_list.json")) {
    Serial.println("No wifi list");
    return;
  }

  File f = SPIFFS.open("/wifi_list.json", "r");
  JsonDocument doc;
  deserializeJson(doc, f);
  f.close();

  wifiList.clear();
  JsonArray array = doc["wifi"].as<JsonArray>();
  for (JsonObject w : array) {
    WifiEntry e;
    e.ssid = w["ssid"].as<String>();
    e.pass = w["pass"].as<String>();
    wifiList.push_back(e);
  }

  Serial.println("Loaded WiFi list:");
  for (auto &w : wifiList)
    Serial.println(" - " + w.ssid);
}

static void connectWiFi() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2.drawStr(0, 20, "WiFi...");
  u8g2.sendBuffer();

  Serial.println("[WiFi] Loading WiFi list...");
  loadWifiList();

  // 1. Multi-WiFi attempt
  for (auto &w : wifiList) {
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

  Serial.println("[WiFi] Multi-WiFi FAILED, opening WiFiManager");

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

static void handleClick() {
  Serial.println("Clicked!");
  brightness += 50;
  if (brightness > 255)
    brightness = 255;
  u8g2.setContrast(brightness);
  saveBrightness();
}

static void handleDoubleclick() {
  Serial.println("Double clicked!");
  brightness -= 50;
  if (brightness < 5)
    brightness = 5;
  u8g2.setContrast(brightness);
  saveBrightness();
}

static void handleTripleClick() {
  Serial.println("Triple clicked!");
  wifiEnabled = !wifiEnabled;
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  if (wifiEnabled) {
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

// saveWifiList moved up

void weatherTask(void *pvParameters) {
  weatherUpdating = true;
  Serial.println("[Weather] Background update started...");

  HTTPClient http;
  http.begin(WEATHER_SERVER);
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
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
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
    if (strcmp(LANG_CODE, "ja") == 0) {
      // Format: 3月11日(水)
      snprintf(buffer, sizeof(buffer), "%s%02d日(%s)", loc->months[timeinfo.tm_mon], timeinfo.tm_mday, loc->days[timeinfo.tm_wday]);
    } else {
      // Format: T4, 11 thg 3
      snprintf(buffer, sizeof(buffer), "%s, %02d%s", loc->days[timeinfo.tm_wday], timeinfo.tm_mday, loc->months[timeinfo.tm_mon]);
    }
    dateStr = String(buffer);
  } else {
    dateStr = chronos.getTime("%d %b");
  }
  u8g2.drawStr(OFFSET_DATE_X, OFFSET_DATE_Y, dateStr.c_str());

  // Bluetooth Icon
  u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
  u8g2.drawGlyph(OFFSET_STATUSBAR_ICON_RIGHT_X, OFFSET_STATUSBAR_ICON_RIGHT_Y, 74);
  if (!chronos.isConnected()) {
    u8g2.drawLine(OFFSET_STATUSBAR_ICON_RIGHT_X, OFFSET_STATUSBAR_ICON_RIGHT_Y, OFFSET_STATUSBAR_ICON_RIGHT_X + 7, OFFSET_STATUSBAR_ICON_RIGHT_Y - 7);
  }

  // Wifi Icon
  u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
  u8g2.drawGlyph(OFFSET_STATUSBAR_ICON_RIGHT_X + 10, OFFSET_STATUSBAR_ICON_RIGHT_Y, 0x0051);
  if (WiFi.status() != WL_CONNECTED) {
    u8g2.drawLine(OFFSET_STATUSBAR_ICON_RIGHT_X + 10 + 1, OFFSET_STATUSBAR_ICON_RIGHT_Y, OFFSET_STATUSBAR_ICON_RIGHT_X + 12 + 7,OFFSET_STATUSBAR_ICON_RIGHT_Y - 7);
  }
}

void drawMainClock() {
  // Left: Clock
  u8g2.setFont(u8g2_font_logisoso24_tn);
  String hourStr, minuteStr, secStr, ampmStr;

  struct tm timeinfo;
  bool timeValid = false;
  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
    timeValid = true;
    char h[3], m[3], s[3], p[3];
    strftime(h, sizeof(h), "%I", &timeinfo); // 12-hour
    strftime(m, sizeof(m), "%M", &timeinfo);
    strftime(s, sizeof(s), "%S", &timeinfo);
    strftime(p, sizeof(p), "%p", &timeinfo);
    hourStr = String(h);
    minuteStr = String(m);
    secStr = String(s);
    ampmStr = String(p);
  } else {
    hourStr = chronos.getHourZ();
    minuteStr = chronos.getTime("%M");
    secStr = chronos.getTime("%S");
    ampmStr = chronos.getAmPmC();
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

  Wire.begin(PIN_SDA, PIN_SCL);

  preferences.begin("settings", true);
  brightness =
      preferences.getInt("light", 150);
  preferences.end();

  u8g2.begin();
  u8g2.setContrast(brightness);
  u8g2.enableUTF8Print();

  button.setup(PIN_TAP, true);
  pinMode(PIN_TAP, INPUT);

  button.attachClick([]() {
    Serial.println("click");
    handleClick();
  });
  button.attachDoubleClick([]() {
    Serial.println("double click");
    handleDoubleclick();
  });
  button.attachMultiClick([](void *s) {
    int clicks = button.getNumberClicks();
    Serial.println("multiple click" + String(clicks));
    if (clicks == 3) {
      handleTripleClick();
    }
  }, NULL);

  chronos.begin();
  if (wifiEnabled) {
    connectWiFi();
  }
}

void loop(void) {
  button.tick();
  
  if (isPortalActive) {
    // WiFiManager is blocking
  } else {
    chronos.loop();

    if (wifiEnabled && WiFi.status() == WL_CONNECTED && !weatherUpdating) {
      if (millis() - lastWeatherUpdate > weatherInterval || lastWeatherUpdate == 0) {
        syncNTP();
        updateOnlineWeather();
      }
    }
    if (chronos.isConnected()) {
      syncLocalFromChronos();
    }

    u8g2.clearBuffer();
    drawStatusBar();
    drawMainClock();
    u8g2.sendBuffer();
  }

  delay(100);
}
