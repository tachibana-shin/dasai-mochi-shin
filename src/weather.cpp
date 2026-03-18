#include "weather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "time_utils.h"

int onlineTemp = -999;
int onlineHumidity = -1;
float onlineWindSpeed = -1.0;
int onlineWeatherCode = -1;
bool onlineIsDay = true;
String onlineSunrise = "--:--";
String onlineSunset = "--:--";
static unsigned long lastWeatherUpdate = 0;
volatile bool weatherUpdating = false;

static void weatherTask(void* pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(20));

  Serial.println("[Weather] Background update started...");

  HTTPClient http;
  http.begin(config.weatherServer);
  http.setTimeout(10000);
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
  vTaskDelete(NULL);
}

static bool updateOnlineWeather() {
  if (weatherUpdating || WiFi.status() != WL_CONNECTED) return false;

  weatherUpdating = true;
  xTaskCreate(weatherTask, "weatherTask", 8192, NULL, 0, NULL);
  return true;
}

void loopWeather() {
  if (config.wifiEnabled && WiFi.status() == WL_CONNECTED && !weatherUpdating) {
    if (millis() - lastWeatherUpdate > config.weatherInterval ||
        lastWeatherUpdate == 0) {
      syncNTP();
      updateOnlineWeather();
    }
  }
}