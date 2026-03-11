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

#define PIN_SDA 20
#define PIN_SCL 21
#define PIN_TAP 10

#define WIFI_AP_NAME "Dasai Mochi Shin"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
OneButton button;
Preferences preferences;
ChronosESP32 chronos("Mochi Shin");

int brightness = 150;
int onlineTemp = 0;
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherInterval = 1800000; // 30 mins
bool wifiEnabled = true;
bool isPortalActive = false;

const char *ntpServer = "time.google.com";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

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

// saveWifiList moved up

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
  Serial.println("weather");
  if (WiFi.status() != WL_CONNECTED)
    return false;

  HTTPClient http;
  // Use wttr.in for a simple JSON response without API key
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin("http://wttr.in/HoChiMinh?format=j1");
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(payload);
    JsonDocument doc;
    deserializeJson(doc, payload);
    onlineTemp = doc["current_condition"][0]["temp_C"].as<int>();
    Serial.println("Updated online weather: " + String(onlineTemp) + "C");
  } else {
    Serial.println("Weather update failed: " + String(httpCode));
  }
  http.end();

  return true;
}

bool syncNTP() {
  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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
  u8g2.drawStr(2, 10, dateStr.c_str());

  // Right: Icons
  int iconX = 100, iconY = 10;
  // bluetooth
  int iconX_BT = 100;
  u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
  u8g2.drawGlyph(iconX, iconY, 74);
  if (!chronos.isConnected()) {
    u8g2.drawLine(iconX, iconY, iconX_BT + 7, iconY - 7);
  }

  iconX = 110;
  // Wifi Icon
  int iconX_WF = 112;
  u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
  u8g2.drawGlyph(iconX, 10, 0x0051);
  if (WiFi.status() != WL_CONNECTED) {
    u8g2.drawLine(iconX + 1, 10, iconX_WF + 7, iconY - 7);
  }
}

void drawMainClock() {
  // Left: Clock
  u8g2.setFont(u8g2_font_logisoso24_tn);
  String hourStr, minuteStr, ampmStr;

  if (WiFi.status() == WL_CONNECTED) {
  struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char h[3], m[3];
      strftime(h, sizeof(h), "%I", &timeinfo); // 12-hour
      strftime(m, sizeof(m), "%M", &timeinfo);
      hourStr = String(h);
      minuteStr = String(m);
      char p[3];
      strftime(p, sizeof(p), "%p", &timeinfo);
      ampmStr = String(p);
    } else {
      hourStr = chronos.getHourZ();
      minuteStr = chronos.getTime("%M");
      ampmStr = chronos.getAmPmC();
    }
  } else {
    hourStr = chronos.getHourZ();
    minuteStr = chronos.getTime("%M");
    ampmStr = chronos.getAmPmC();
  }

  String timeStr = hourStr + ":" + minuteStr;
  u8g2.drawStr(2, 45, timeStr.c_str());

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(72, 45, ampmStr.c_str());

  // Right: Weather
  int weatherX = 90;
  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
  u8g2.drawGlyph(weatherX, 40, 0x45); // Sun icon

  u8g2.setFont(u8g2_font_ncenB10_tr);
  int temp = 0;
  if (WiFi.status() == WL_CONNECTED) {
    temp = onlineTemp;
  } else if (chronos.getWeatherCount() > 0) {
    temp = chronos.getWeatherAt(0).temp;
  }
  String tempStr = String(temp) + "°C";
  u8g2.drawStr(weatherX, 55, tempStr.c_str());
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
