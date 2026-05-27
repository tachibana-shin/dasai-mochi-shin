#include "wifi_manager.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>

#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "router.h"
#include "web_server.h"
#include "time_utils.h"

namespace {
constexpr uint32_t kConfigPortalTimeoutSec = 180;

bool tryWifiEntry(const String& ssid, const String& pass) {
  if (ssid.length() == 0) return false;

  Serial.printf("[WiFi] Trying: %s\n", ssid.c_str());

  WiFi.persistent(true);
  WiFi.enableSTA(true);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    retry++;
  }
  WiFi.persistent(false);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Connected to %s\n", ssid.c_str());
    return true;
  }

  WiFi.disconnect(false, false);
  return false;
}

bool tryConfigWifiEntries() {
  for (auto& w : config.wifi) {
    if (tryWifiEntry(w.ssid, w.pass)) return true;
  }
  return false;
}
}

void initWiFi() {
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_MODE_APSTA);

  wifi_config_t cfg;
  esp_wifi_get_config(WIFI_IF_AP, &cfg);
  cfg.ap.channel = 1;
  cfg.ap.max_connection = 4;
  esp_wifi_set_config(WIFI_IF_AP, &cfg);

  if (config.wifiEnabled) {
    showMessage(("WiFi Setup\nAP: " + config.wifiAPName).c_str(), 0);

    if (tryConfigWifiEntries()) {
      ensureMdnsStarted();
      WiFi.mode(WIFI_STA);
      String ip = WiFi.localIP().toString();
      Serial.printf("WiFi connected! IP: http://%s\n", ip.c_str());
      showMessage(("IP: " + ip).c_str(), 3000);
      clearMessage();
      return;
    }

    Serial.println("[WiFi] Saved entries failed, opening WiFiManager");
    WiFiManager wm;
    wm.setClass("invert");
    wm.setConfigPortalTimeout(kConfigPortalTimeoutSec);
    wm.setConnectTimeout(10);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setSleep(true);
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

    bool connected = wm.autoConnect(config.wifiAPName.c_str());
    if (connected) {
      ensureMdnsStarted();
      WiFi.mode(WIFI_STA);
      String ip = WiFi.localIP().toString();
      Serial.printf("WiFi connected! IP: http://%s\n", ip.c_str());
      showMessage(("IP: " + ip).c_str(), 3000);
    }
    clearMessage();
  }
}

void loopWiFiManager() {
  u8g2->clearBuffer();
  u8g2->setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2->drawStr(0, 20, "WiFi...");
  sendBuffer();

  Serial.println("[WiFi] Attempting connections from config...");

  if (tryConfigWifiEntries()) {
    Serial.println("[WiFi] Connected by saved list!");
    ensureMdnsStarted();
    String msg = String(L(MSG_WIFI_CONNECTED)) + ":\n" + WiFi.SSID();
    showMessage(msg.c_str(), 1500);
    syncNTP();
    Router::pop();
    return;
  }

  Serial.println("[WiFi] All saved failed, opening WiFiManager");

  showMessage(("WiFi Setup\nAP: " + config.wifiAPName).c_str(), 0);

  WiFiManager wm;
  wm.setClass("invert");
  wm.setConnectTimeout(10);
  wm.setConfigPortalTimeout(kConfigPortalTimeoutSec);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  bool ok = wm.autoConnect(config.wifiAPName.c_str());
  if (!ok) {
    Serial.println("[WiFiManager] Failed or timeout");
    showMessage(L(MSG_WIFI_FAILED), 1500);
  } else {
    ensureMdnsStarted();
    String msg = String(L(MSG_WIFI_CONNECTED)) + ":\n" + wm.getWiFiSSID();
    showMessage(msg.c_str(), 1500);
  }

  syncNTP();
  Router::pop();
}
