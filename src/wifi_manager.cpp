#include "wifi_manager.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>

#include "config.h"
#include "display.h"

bool isPortalActive = false;

void initWiFi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_MODE_APSTA);

  wifi_config_t cfg;
  esp_wifi_get_config(WIFI_IF_AP, &cfg);
  cfg.ap.channel = 1;
  cfg.ap.max_connection = 4;
  esp_wifi_set_config(WIFI_IF_AP, &cfg);

  if (config.wifiEnabled) {
    WiFiManager wm;
    wm.setClass("invert");
    wm.setTimeout(180);
    wm.setConfigPortalTimeout(180);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    wm.setConfigPortalBlocking(false);
    wm.autoConnect(config.wifiAPName.c_str());
    wm.setConfigPortalBlocking(true);
  }
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

bool openWiFiManager() {
  isPortalActive = true;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2.drawStr(0, 20, "WiFi Setup");
  sendBuffer();

  WiFiManager wm;
  wm.setClass("invert");
  wm.setTimeout(180);
  wm.setConfigPortalTimeout(180);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  u8g2.setCursor(0, 40);
  u8g2.print("AP: ");
  u8g2.print(config.wifiAPName);
  sendBuffer();

  bool ok = wm.autoConnect(config.wifiAPName.c_str());
  isPortalActive = false;

  if (!ok) {
    Serial.println("[WiFiManager] Failed or timeout");
    return false;
  }

  return true;
}

void connectWiFi() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2.drawStr(0, 20, "WiFi...");
  sendBuffer();

  Serial.println("[WiFi] Attempting connections from config...");

  // for (auto& w : config.wifi) {
  //   Serial.println("[WiFi] Trying: " + w.ssid);
  //   if (wifiConnectNew(w.ssid, w.pass, true)) {
  //     int retry = 0;
  //     while (WiFi.status() != WL_CONNECTED && retry < 20) {
  //       delay(500);
  //       retry++;
  //     }
  //     if (WiFi.status() == WL_CONNECTED) {
  //       Serial.println("[WiFi] Connected by saved list!");
  //       return;
  //     }
  //   }
  // }

  Serial.println("[WiFi] All saved failed, opening WiFiManager");
  if (!openWiFiManager()) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "WiFi Failed");
    sendBuffer();
    delay(1500);
    return;
  }
}