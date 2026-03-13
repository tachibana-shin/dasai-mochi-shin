#include "display.h"

#include <ChronosESP32.h>
#include <Wifi.h>
#include <Wire.h>
#include <time.h>

#include "chronos_manager.h"
#include "config.h"
#include "e_locale.h"
#include "time_utils.h"
#include "weather.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

bool screenOn = true;
static unsigned long lastScreenAutoCheck = 0;
const unsigned long screenAutoCheckInterval = 60000;

void initDisplay() {
  Wire.begin(config.pinScreenSDA, config.pinScreenSCL);
  u8g2.begin();
  u8g2.setContrast(config.brightness);
  u8g2.enableUTF8Print();
}

void toggleScreen() {
  screenOn = !screenOn;
  u8g2.setPowerSave(!screenOn);
  Serial.println(screenOn ? "Screen ON" : "Screen OFF");
}

void showMessage(const char* msg, uint32_t timeout) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  u8g2.drawStr(0, 20, msg);
  sendBuffer();
  delay(timeout);
}

static void checkScreenAutoOff() {
  if (millis() - lastScreenAutoCheck < screenAutoCheckInterval) return;
  lastScreenAutoCheck = millis();

  int onHour = config.autoOnHour;
  int offHour = config.autoOffHour;

  if (onHour == -1 && offHour == -1) return;

  int currentHour = -1;
  struct tm timeinfo;

  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
    currentHour = timeinfo.tm_hour;
  } else {
    currentHour = chronos.getHour();
  }

  if (currentHour < 0 || currentHour > 23) return;

  bool shouldBeOn;

  if (onHour != -1 && offHour != -1) {
    if (onHour <= offHour) {
      shouldBeOn = (currentHour >= onHour && currentHour < offHour);
    } else {
      shouldBeOn = (currentHour >= onHour || currentHour < offHour);
    }
  } else if (onHour != -1) {
    shouldBeOn = (currentHour >= onHour);
  } else {
    shouldBeOn = (currentHour < offHour);
  }

  if (shouldBeOn && !screenOn) {
    toggleScreen();
  } else if (!shouldBeOn && screenOn) {
    toggleScreen();
  }
}

void loopDisplay() { checkScreenAutoOff(); }
void sendBuffer() {
  if (config.screenNegative) {
    uint8_t* buf = u8g2.getBufferPtr();
    const uint16_t len =
        (uint16_t)(config.screenWidth / 8) * config.screenHeight;

    for (uint16_t i = 0; i < len; i++) {
      buf[i] ^= 0xFF;
    }
  }

  // With default R0: apply 180 rotation only when flip mode is on
  if (config.screenFlipMode) {
    uint8_t* buf = u8g2.getBufferPtr();
    const uint16_t len =
        (uint16_t)(config.screenWidth / 8) * config.screenHeight;
    for (uint16_t i = 0; i < len / 2; i++) {
      uint8_t tmp = buf[i];
      buf[i] = buf[len - 1 - i];
      buf[len - 1 - i] = tmp;
    }
    for (uint16_t i = 0; i < len; i++) {
      uint8_t b = buf[i];
      b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
      b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
      b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
      buf[i] = b;
    }
  }

  u8g2.sendBuffer();
}