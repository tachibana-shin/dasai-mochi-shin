#include "menu.h"

#include <WiFi.h>

#include <vector>

#include "audio_player.h"
#include "chronos_manager.h"
#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "router.h"

static std::vector<MenuItem> menuItems;
static int selectedIndex = 0;
static bool initialized = false;

static void setupMenuItems() {
  menuItems.clear();

  // Back
  MenuItem back(L(MSG_BACK), MenuItemType::BACK);
  back.onAction = []() { Router::pop(); };
  menuItems.push_back(back);

  // Brightness
  MenuItem brightness(L(MSG_BRIGHTNESS), MenuItemType::RANGE);
  brightness.getValue = []() { return String(config.brightness); };
  brightness.onAction = []() {
    config.brightness = (config.brightness + 50) % 300;
    if (config.brightness < 5) config.brightness = 5;
    if (config.brightness > 255) config.brightness = 255;
    u8g2->setContrast(config.brightness);
    saveConfig();
  };
  menuItems.push_back(brightness);

  // WiFi
  MenuItem wifi(L(MSG_WIFI), MenuItemType::TOGGLE);
  wifi.getValue = []() -> String {
    if (WiFi.status() == WL_CONNECTED) {
      return String("ON (") + WiFi.SSID() + ")";
    }
    return config.wifiEnabled ? String("ON") : String("OFF");
  };
  wifi.onAction = []() {
    config.wifiEnabled = !config.wifiEnabled;
    if (!config.wifiEnabled) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    } else {
      Router::push(Route::WIFI_MANAGER);
    }
    saveConfig();
  };
  menuItems.push_back(wifi);

  // Bluetooth
  MenuItem bt(L(MSG_BLUETOOTH), MenuItemType::TOGGLE);
  bt.getValue = []() {
    if (config.bluetoothEnabled && chronos.isConnected()) {
      return String("ON (C)");
    }
    return config.bluetoothEnabled ? String("ON") : String("OFF");
  };
  bt.onAction = []() {
    config.bluetoothEnabled = !config.bluetoothEnabled;
    saveConfig();
    // Requires reboot or re-init chronos
  };
  menuItems.push_back(bt);

  // Language
  MenuItem lang(L(MSG_LANGUAGE), MenuItemType::ACTION);
  lang.getValue = []() { return config.langCode; };
  lang.onAction = []() {
    config.langCode = (config.langCode == "vi") ? "en" : "vi";
    saveConfig();
  };
  menuItems.push_back(lang);

  // GMT Offset
  MenuItem gmt(L(MSG_GMT_OFFSET), MenuItemType::RANGE);
  gmt.getValue = []() { return String(config.gmtOffset_sec / 3600); };
  gmt.onAction = []() {
    int offset = config.gmtOffset_sec / 3600;
    offset = (offset + 1);
    if (offset > 12) offset = -12;
    config.gmtOffset_sec = offset * 3600;
    saveConfig();
  };
  menuItems.push_back(gmt);

  // Flip Screen
  MenuItem flip(L(MSG_FLIP_SCREEN), MenuItemType::TOGGLE);
  flip.getValue = []() {
    return config.screenFlipMode ? String("ON") : String("OFF");
  };
  flip.onAction = []() {
    config.screenFlipMode = !config.screenFlipMode;
    saveConfig();
  };
  menuItems.push_back(flip);

  // Negative Screen
  MenuItem neg(L(MSG_NEGATIVE), MenuItemType::TOGGLE);
  neg.getValue = []() {
    return config.screenNegative ? String("ON") : String("OFF");
  };
  neg.onAction = []() {
    config.screenNegative = !config.screenNegative;
    saveConfig();
  };
  menuItems.push_back(neg);

  // Auto Off Hour
  MenuItem offH(L(MSG_AUTO_OFF_H), MenuItemType::RANGE);
  offH.getValue = []() { return String(config.autoOffHour); };
  offH.onAction = []() {
    config.autoOffHour = (config.autoOffHour + 1) % 24;
    saveConfig();
  };
  menuItems.push_back(offH);

  // Auto On Hour
  MenuItem onH(L(MSG_AUTO_ON_H), MenuItemType::RANGE);
  onH.getValue = []() { return String(config.autoOnHour); };
  onH.onAction = []() {
    config.autoOnHour = (config.autoOnHour + 1) % 24;
    saveConfig();
  };
  menuItems.push_back(onH);

  // Weather Interval
  MenuItem weatherInt(L(MSG_WEATHER_INT), MenuItemType::RANGE);
  weatherInt.getValue = []() {
    return String(config.weatherInterval / 60000) + "m";
  };
  weatherInt.onAction = []() {
    int minutes = config.weatherInterval / 60000;
    minutes = (minutes + 15) % 120;  // 15, 30, 45...
    if (minutes == 0) minutes = 15;
    config.weatherInterval = (unsigned long)minutes * 60000;
    saveConfig();
  };
  menuItems.push_back(weatherInt);

  // Mochi Speed
  MenuItem mSpeed(L(MSG_MOCHI_SPD), MenuItemType::RANGE);
  mSpeed.getValue = []() { return String(config.mochiSpeedDivisor); };
  mSpeed.onAction = []() {
    config.mochiSpeedDivisor = (config.mochiSpeedDivisor % 5) + 1;
    saveConfig();
  };
  menuItems.push_back(mSpeed);

  // Mochi Negative
  MenuItem mNeg(L(MSG_MOCHI_NEG), MenuItemType::TOGGLE);
  mNeg.getValue = []() {
    return config.mochiNegative ? String("ON") : String("OFF");
  };
  mNeg.onAction = []() {
    config.mochiNegative = !config.mochiNegative;
    saveConfig();
  };
  menuItems.push_back(mNeg);

  // Time Format (12h/24h)
  MenuItem tForm(L(MSG_TIME_FORMAT), MenuItemType::TOGGLE);
  tForm.getValue = []() {
    return config.is24Hour ? String("24H") : String("12H");
  };
  tForm.onAction = []() {
    config.is24Hour = !config.is24Hour;
    saveConfig();
  };
  menuItems.push_back(tForm);

  // Mochi Clock Interval
  MenuItem mClockInt(L(MSG_MOCHI_CLK_INT), MenuItemType::RANGE);
  mClockInt.getValue = []() {
    return String(config.mochiClockInterval / 60000) + "m";
  };
  mClockInt.onAction = []() {
    int mins = config.mochiClockInterval / 60000;
    mins = (mins % 30) + 1;  // 1 to 30 mins
    config.mochiClockInterval = (unsigned long)mins * 60000;
    saveConfig();
  };
  menuItems.push_back(mClockInt);

  // Mochi Clock Duration
  MenuItem mClockDur(L(MSG_MOCHI_CLK_DUR), MenuItemType::RANGE);
  mClockDur.getValue = []() {
    return String(config.mochiClockDuration / 1000) + "s";
  };
  mClockDur.onAction = []() {
    int secs = config.mochiClockDuration / 1000;
    secs = (secs % 30) + 5;  // 5 to 30 secs
    if (secs > 30) secs = 5;
    config.mochiClockDuration = (unsigned long)secs * 1000;
    saveConfig();
  };
}

void initMenu() {
  setupMenuItems();
  selectedIndex = 0;
  initialized = true;
}

void loopSettings() {
  if (!initialized) initMenu();

  u8g2->clearBuffer();
  u8g2->setFont(u8g2_font_unifont_t_vietnamese1);

  int displayW = u8g2->getDisplayWidth();
  int displayH = u8g2->getDisplayHeight();

  int itemHeight = vh(25);  // 16px if height is 64px
  if (itemHeight < 12) itemHeight = 12;
  int visibleCount = displayH / itemHeight;
  int scrollOffset = 0;

  if (selectedIndex >= visibleCount) {
    scrollOffset = selectedIndex - visibleCount + 1;
  }

  // Draw scrollbar
  int scrollBarHeight = displayH;
  int barWidth = rem(0.4);
  if (barWidth < 3) barWidth = 3;
  int barX = displayW - barWidth;

  int barSize = (visibleCount * scrollBarHeight) / menuItems.size();
  if (barSize < 4) barSize = 4;
  int barPos =
      (selectedIndex * (scrollBarHeight - barSize)) / (menuItems.size() - 1);

  u8g2->drawVLine(displayW - 2, 0, scrollBarHeight);
  u8g2->drawBox(barX, barPos, barWidth, barSize);

  int usableWidth = barX - 2;

  for (int i = 0; i < visibleCount; i++) {
    int idx = i + scrollOffset;
    if (idx >= menuItems.size()) break;

    int y = (i + 1) * itemHeight - rem(0.4);

    if (idx == selectedIndex) {
      u8g2->setDrawColor(1);
      u8g2->drawBox(0, i * itemHeight, usableWidth + 1, itemHeight);
      u8g2->setDrawColor(0);
    } else {
      u8g2->setDrawColor(1);
    }

    // --- Logic calculating widths ---
    String val = "";
    int realValWidth = 0;
    if (menuItems[idx].getValue) {
      val = menuItems[idx].getValue();
      realValWidth = u8g2->getUTF8Width(val.c_str());
    }

    int maxValWidth = vw(37.5);  // Exact 48px on 128px screen
    int displayValWidth =
        (realValWidth > maxValWidth) ? maxValWidth : realValWidth;
    int availableLabelWidth =
        usableWidth - (displayValWidth > 0 ? (displayValWidth + rem(0.8)) : 0) -
        rem(0.5);
    int labelWidth = u8g2->getUTF8Width(menuItems[idx].label.c_str());

    // --- Draw Label ---
    u8g2->setClipWindow(rem(0.25), i * itemHeight,
                        availableLabelWidth + rem(0.25), (i + 1) * itemHeight);
    if (idx == selectedIndex && labelWidth > availableLabelWidth) {
      int scrollArea = labelWidth + rem(4);
      int offset = (millis() / 40) % scrollArea;
      u8g2->drawUTF8(rem(0.25) - offset, y, menuItems[idx].label.c_str());
      if (offset > (scrollArea - availableLabelWidth)) {
        u8g2->drawUTF8(rem(0.3) - offset + scrollArea, y,
                       menuItems[idx].label.c_str());
      }
    } else {
      u8g2->drawUTF8(rem(0.3), y, menuItems[idx].label.c_str());
    }
    u8g2->setMaxClipWindow();

    // --- Draw Value ---
    if (val.length() > 0) {
      int valStartX = usableWidth - maxValWidth;
      u8g2->setClipWindow(valStartX, i * itemHeight, usableWidth,
                          (i + 1) * itemHeight);

      if (idx == selectedIndex && realValWidth > maxValWidth) {
        int scrollArea = realValWidth + rem(2.5);
        int offset = (millis() / 40) % scrollArea;
        u8g2->drawUTF8(valStartX - offset, y, val.c_str());
        if (offset > (scrollArea - maxValWidth)) {
          u8g2->drawUTF8(valStartX - offset + scrollArea, y, val.c_str());
        }
      } else {
        // Alignment: if text is shorter than max area, align right. │ If
        // longer, start from beginning (left of area).
        int drawX = (realValWidth > maxValWidth) ? valStartX
                                                 : (usableWidth - realValWidth);
        u8g2->drawUTF8(drawX, y, val.c_str());
      }
      u8g2->setMaxClipWindow();
    }
  }
  u8g2->setDrawColor(1);
  sendBuffer();
}

void handleMenuClick() {
  audioPlayDefault(SOUND_CLICK);
  selectedIndex = (selectedIndex + 1) % menuItems.size();
}

void handleMenuDoubleClick() {
  audioPlayDefault(SOUND_CLICK);
  if (selectedIndex < menuItems.size() && menuItems[selectedIndex].onAction) {
    menuItems[selectedIndex].onAction();
  }
}

void handleMenuTripleClick() { Router::pop(); }
