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

#define WEATHER_SERVER "http://api.open-meteo.com/v1/forecast?latitude=10.762622&longitude=106.660172&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code,is_day&daily=sunrise,sunset&timezone=auto"

String onlineSunrise = "";
String onlineSunset = "";

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
int onlineTemp = 0;
int onlineHumidity = 0;
float onlineWindSpeed = 0;
int onlineWeatherCode = 0;
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
  preferences.begin("settings", false);    // Mở không gian "settings"
  preferences.putInt("light", brightness); // Lưu giá trị với khóa "light"
  preferences.end();
  Serial.println("Đã lưu độ sáng vào Flash!");
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

// 📌 Show WiFi setup portal nếu Multi-WiFi thất bại
bool openWiFiManager() {
  isPortalActive = true;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2.drawStr(0, 20, "WiFi Setup");
  u8g2.sendBuffer();

  WiFiManager wm;
  wm.setClass("invert");          // giao diện tối, đẹp cho OLED
  wm.setTimeout(180);             // 3 phút auto exit
  wm.setConfigPortalTimeout(180); // nếu không ai cấu hình thì thoát
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

  // Lưu WiFi mới vào wifi_list.json
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

  // 2. Mở WiFiManager portal
  if (!openWiFiManager()) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "WiFi Failed");
    u8g2.sendBuffer();
    delay(1500);
    return;
  }

  // 3. Kết nối lại bằng dữ liệu mới
  Serial.println("[WiFi] Reconnecting with new credentials...");
  wifiConnectNew(WiFi.SSID(), WiFi.psk(), true);
}

// Hàm xử lý khi nhấn 1 chạm (Tăng độ sáng)
static void handleClick() {
  Serial.println("Clicked!");
  brightness += 50;
  if (brightness > 255)
    brightness = 255;
  u8g2.setContrast(brightness);
  saveBrightness();
}

// Hàm xử lý khi nhấn 2 chạm (Giảm độ sáng)
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

bool updateOnlineWeather() {
  if (WiFi.status() != WL_CONNECTED)
    return false;

  HTTPClient http;
  // Use Open-Meteo API for HCMC (Latitude: 10.762622, Longitude: 106.660172)
  http.begin(WEATHER_SERVER);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    deserializeJson(doc, payload);
    onlineTemp = doc["current"]["temperature_2m"].as<int>();
    onlineHumidity = doc["current"]["relative_humidity_2m"].as<int>();
    onlineWindSpeed = doc["current"]["wind_speed_10m"].as<float>();
    onlineWeatherCode = doc["current"]["weather_code"].as<int>();
    onlineIsDay = doc["current"]["is_day"].as<int>() == 1;

    // Parse Sunrise / Sunset
    // Format is "2024-xx-xxT06:xx"
    String sr = doc["daily"]["sunrise"][0].as<String>();
    String ss = doc["daily"]["sunset"][0].as<String>();
    if (sr.length() > 11) onlineSunrise = sr.substring(11);
    if (ss.length() > 11) onlineSunset = ss.substring(11);
  } else {
    Serial.println("Weather update failed: " + String(httpCode));
    http.end();
    return false;
  }
  http.end();

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
  u8g2.setFont(u8g2_font_6x10_tf);

  // Left: Date
  String dateStr;
  struct tm timeinfo;
  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%d %b", &timeinfo);
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

  String timeStr = hourStr + ":" + minuteStr;
  u8g2.drawStr(OFFSET_TIME_X, OFFSET_TIME_Y, timeStr.c_str()); // Corrected from OFFSET_Y_TIME

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(OFFSET_SEC_X, OFFSET_SEC_Y, secStr.c_str()); // Seconds above AM/PM
  u8g2.drawStr(OFFSET_SEC_X, OFFSET_SEC_Y + 10, ampmStr.c_str());

  // Right: Weather Icon
  
  uint8_t iconGlyph = 0x45; // Default: Sun
  int weatherCode = 0;
  bool isDay = true;

  if (WiFi.status() == WL_CONNECTED) {
    weatherCode = onlineWeatherCode;
    isDay = onlineIsDay;
  }
  
  // WMO Code Mapping to Open Iconic Weather
  if (weatherCode == 0) iconGlyph = isDay ? 0x45 : 0x44; // Sun / Moon
  else if (weatherCode == 1 || weatherCode == 2) iconGlyph = 0x46; // Cloud + Sun
  else if (weatherCode == 3 || weatherCode == 45 || weatherCode == 48) iconGlyph = 0x41; // Cloud
  else if ((weatherCode >= 51 && weatherCode <= 65) || (weatherCode >= 80 && weatherCode <= 82)) iconGlyph = 0x43; // Rain
  else if ((weatherCode >= 71 && weatherCode <= 77) || (weatherCode >= 85 && weatherCode <= 86)) iconGlyph = 0x42; // Snow/Umbrella
  else if (weatherCode >= 95) iconGlyph = 0x47; // Thunder

  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
  u8g2.drawGlyph(OFFSET_WEATHER_ICON_X, OFFSET_WEATHER_ICON_Y, iconGlyph); 

  u8g2.setFont(u8g2_font_ncenB10_tr);
  int temp = 0;
  int humidity = 0;
  float wind = 0;

  if (WiFi.status() == WL_CONNECTED) {
    temp = onlineTemp;
    humidity = onlineHumidity;
    wind = onlineWindSpeed;
  } else if (chronos.getWeatherCount() > 0) {
    temp = chronos.getWeatherAt(0).temp;
    // ChronosESP32 might not provide humidity/wind easily in this snippet, 
    // but we'll show what we have.
  }

  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  String tempStr = String(temp) + "C";
  u8g2.drawStr(OFFSET_WEATHER_TEMP_X, OFFSET_WEATHER_TEMP_Y, tempStr.c_str());

  // Sunrise / Sunset at the bottom
  if (onlineSunrise != "" && onlineSunset != "") {
    u8g2.setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2.drawGlyph(2, 63, 0x45); // Sun icon for rise
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(12, 63, onlineSunrise.c_str());

    u8g2.setFont(u8g2_font_open_iconic_weather_1x_t);
    u8g2.drawGlyph(65, 63, 0x44); // Moon/Sunset icon
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(75, 63, onlineSunset.c_str());
  }

  // Humidity and Wind (Small)
  u8g2.setFont(u8g2_font_5x7_tf);
  String humStr = "H" + String(humidity) + "%";
  String windStr = "W" + String(wind, 1) + "m/s";
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
  cfg.ap.channel = 1;            // ép kênh để tránh lỗi C3 hay bị đứng
  cfg.ap.max_connection = 4;
  esp_wifi_set_config(WIFI_IF_AP, &cfg);

  // Cấu hình chân I2C cho ESP32-C3
  Wire.begin(PIN_SDA, PIN_SCL);

  preferences.begin("settings", true); // Mở ở chế độ chỉ đọc (read-only)
  brightness =
      preferences.getInt("light", 150); // Mặc định 150 nếu chưa có dữ liệu
  preferences.end();

  // Khởi tạo màn hình
  u8g2.begin();
  u8g2.setContrast(brightness);
  u8g2.enableUTF8Print();

  // Cấu hình OneButton theo cách "Explicit setup" (khuyên dùng cho ESP32)
  button.setup(PIN_TAP, INPUT_PULLUP, true);

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

    if (wifiEnabled && WiFi.status() == WL_CONNECTED) {
      if (millis() - lastWeatherUpdate > weatherInterval || lastWeatherUpdate == 0) {
        if (syncNTP() && updateOnlineWeather()) {
          lastWeatherUpdate = millis();
        }
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
}
