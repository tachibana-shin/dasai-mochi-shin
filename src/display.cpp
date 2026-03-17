#include "display.h"
#include "audio_manager.h"

#include <ChronosESP32.h>
#include <Wifi.h>
#include <Wire.h>
#include <string.h>
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
  playLockscreenAudio();
  Serial.println(screenOn ? "Screen ON" : "Screen OFF");
}

void showMessage(const char *msg, uint32_t timeout, uint8_t mode) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese1);

  int16_t width = u8g2.getDisplayWidth();
  int16_t height = u8g2.getDisplayHeight();
  int16_t y = 20;
  int16_t lineHeight = u8g2.getMaxCharHeight();

  if (mode == SHOW_WRAP) {
    char *lastSpace = NULL;
    int16_t lineStart = 0;
    int16_t i = 0;

    while (msg[i] != '\0') {
      char ch = msg[i];
      bool forceNewlne = (ch == '\n');
      
      if (ch == ' ' || ch == '\t' || ch == '\n') {
        lastSpace = (char *)&msg[i];
      }

      int len = i - lineStart + (forceNewlne ? 0 : 1);
      char temp[64];
      int16_t segmentWidth = 0;
      if (len > 0 && len < 64) {
        strncpy(temp, msg + lineStart, len);
        temp[len] = '\0';
        segmentWidth = u8g2.getStrWidth(temp);
      }

      if (forceNewlne || segmentWidth > width) {
        int lineLen = 0;
        if (forceNewlne) {
          lineLen = i - lineStart;
        } else if (lastSpace != NULL && lastSpace > msg + lineStart) {
          lineLen = lastSpace - (msg + lineStart);
        } else {
          lineLen = i - lineStart;
        }

        if (lineLen > 0) {
          char *lineBuf = (char *)malloc(lineLen + 1);
          if (lineBuf) {
            strncpy(lineBuf, msg + lineStart, lineLen);
            lineBuf[lineLen] = '\0';
            u8g2.drawStr(0, y, lineBuf);
            free(lineBuf);
          }
        }

        if (forceNewlne) {
          lineStart = i + 1;
          i = lineStart;
        } else if (lastSpace != NULL && lastSpace > msg + lineStart) {
          lineStart = (lastSpace - msg) + 1;
          i = lineStart;
        } else {
          lineStart = i;
        }
        
        lastSpace = NULL;
        y += lineHeight;
        if (y + lineHeight > height) break;
      } else {
        i++;
      }
    }

    if (msg[lineStart] != '\0') {
      u8g2.drawStr(0, y, msg + lineStart);
    }
  } else if (mode == SHOW_MARQUEE) {
    int16_t textWidth = u8g2.getStrWidth(msg);
    int16_t x = width;
    uint32_t startTime = millis();
    uint32_t stepTime = 30;
    int16_t step = 2;

    while (millis() - startTime < timeout) {
      u8g2.clearBuffer();
      u8g2.drawStr(x, y, msg);
      u8g2.sendBuffer();
      x -= step;
      if (x < -textWidth) {
        x = width;
      }
      delay(stepTime);
    }
  } else {
    u8g2.drawStr(0, y, msg);
  }

  u8g2.sendBuffer();
  delay(timeout);
}

static void checkScreenAutoOff() {
  if (millis() - lastScreenAutoCheck < screenAutoCheckInterval) return;
  lastScreenAutoCheck = millis();

  int onHour = config.autoOnHour;
  int offHour = config.autoOffHour;

  if (onHour == -1 && offHour == -1) return;

  int currentHour = getHour24();

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
    uint8_t *buf = u8g2.getBufferPtr();
    const uint16_t len =
        (uint16_t)(u8g2.getDisplayWidth() / 8) * u8g2.getDisplayHeight();

    for (uint16_t i = 0; i < len; i++) {
      buf[i] ^= 0xFF;
    }
  }

  // With default R0: apply 180 rotation only when flip mode is on
  if (config.screenFlipMode) {
    uint8_t *buf = u8g2.getBufferPtr();
    const uint16_t len =
        (uint16_t)(u8g2.getDisplayWidth() / 8) * u8g2.getDisplayHeight();
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