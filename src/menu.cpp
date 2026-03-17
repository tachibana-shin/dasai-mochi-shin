#include "menu.h"

#include <WiFi.h>

#include <vector>

#include "alarm.h"
#include "audio_manager.h"
#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "reminder.h"
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
    u8g2.setContrast(config.brightness);
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
    extern ChronosESP32 chronos;
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

  // Drink Reminder
  MenuItem drink(L(MSG_DRINK_REM), MenuItemType::TOGGLE);
  drink.getValue = []() {
    return config.drink.enabled ? String("ON") : String("OFF");
  };
  drink.onAction = []() {
    config.drink.enabled = !config.drink.enabled;
    saveConfig();
    if (config.drink.enabled) initReminder();
  };
  menuItems.push_back(drink);

  // Alarm (Simplistic toggle for first alarm)
  MenuItem alarm(L(MSG_ALARM), MenuItemType::TOGGLE);
  alarm.getValue = []() {
    if (config.alarms.empty()) return String("N/A");
    char buf[16];
    sprintf(buf, "%02d:%02d %s", config.alarms[0].hour, config.alarms[0].minute,
            config.alarms[0].enabled ? "ON" : "OFF");
    return String(buf);
  };
  alarm.onAction = []() {
    if (config.alarms.empty()) {
      AlarmEntry e;
      e.enabled = true;
      e.hour = 7;
      e.minute = 0;
      config.alarms.push_back(e);
    } else {
      config.alarms[0].enabled = !config.alarms[0].enabled;
    }
    saveConfig();
  };
  menuItems.push_back(alarm);

  // Alarm Repeat
  MenuItem alarmRep(L(MSG_ALARM_REP), MenuItemType::ACTION);
  alarmRep.getValue = []() {
    if (config.alarms.empty()) return String("N/A");
    uint8_t r = config.alarms[0].repeat;
    if (r == 0 || r == 127) return String("Daily");
    if (r == 31) return String("Mon-Fri");
    if (r == 96) return String("Weekend");
    if (r == 128) return String("Once");
    return String("Custom");
  };
  alarmRep.onAction = []() {
    if (config.alarms.empty()) return;
    uint8_t& r = config.alarms[0].repeat;
    if (r == 0 || r == 127)
      r = 31;  // To Weekdays
    else if (r == 31)
      r = 96;  // To Weekend
    else if (r == 96)
      r = 128;  // To Once
    else
      r = 127;  // Back to Daily
    saveConfig();
  };
  menuItems.push_back(alarmRep);

  // Alarm Hour
  MenuItem alarmH(L(MSG_ALARM_HOUR), MenuItemType::RANGE);
  alarmH.getValue = []() {
    if (config.alarms.empty()) return String("N/A");
    return String(config.alarms[0].hour);
  };
  alarmH.onAction = []() {
    if (config.alarms.empty()) return;
    config.alarms[0].hour = (config.alarms[0].hour + 1) % 24;
    saveConfig();
  };
  menuItems.push_back(alarmH);

  // Alarm Minute
  MenuItem alarmM(L(MSG_ALARM_MIN), MenuItemType::RANGE);
  alarmM.getValue = []() {
    if (config.alarms.empty()) return String("N/A");
    return String(config.alarms[0].minute);
  };
  alarmM.onAction = []() {
    if (config.alarms.empty()) return;
    config.alarms[0].minute = (config.alarms[0].minute + 5) % 60;
    saveConfig();
  };
  menuItems.push_back(alarmM);

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

  // Alarm Sound
  MenuItem aSound(L(MSG_ALARM_SOUND), MenuItemType::TOGGLE);
  aSound.getValue = []() {
    return config.audio.alarmEnabled ? String("ON") : String("OFF");
  };
  aSound.onAction = []() {
    config.audio.alarmEnabled = !config.audio.alarmEnabled;
    saveConfig();
  };
  menuItems.push_back(aSound);

  // Drink Sound
  MenuItem dSound(L(MSG_DRINK_SOUND), MenuItemType::TOGGLE);
  dSound.getValue = []() {
    return config.audio.drinkEnabled ? String("ON") : String("OFF");
  };
  dSound.onAction = []() {
    config.audio.drinkEnabled = !config.audio.drinkEnabled;
    saveConfig();
  };
  menuItems.push_back(dSound);

  // Notify Sound
  MenuItem nSound(L(MSG_NOTIFY_SOUND), MenuItemType::TOGGLE);
  nSound.getValue = []() {
    return config.audio.notifyEnabled ? String("ON") : String("OFF");
  };
  nSound.onAction = []() {
    config.audio.notifyEnabled = !config.audio.notifyEnabled;
    saveConfig();
  };
  menuItems.push_back(nSound);

  // Volume
  MenuItem vol(L(MSG_VOLUME), MenuItemType::RANGE);
  vol.getValue = []() { return String(config.audio.volume); };
  vol.onAction = []() {
    config.audio.volume = (config.audio.volume + 3) % 22;
    setVolume(config.audio.volume);
    saveConfig();
  };
  menuItems.push_back(vol);

  // Time Format (12h/24h)
  MenuItem tForm(L(MSG_TIME_FORMAT), MenuItemType::TOGGLE);
  tForm.getValue = []() { return config.is24Hour ? String("24H") : String("12H"); };
  tForm.onAction = []() {
    config.is24Hour = !config.is24Hour;
    saveConfig();
  };
  menuItems.push_back(tForm);

  // Drink Start Hour
  MenuItem dStart(L(MSG_DRINK_START), MenuItemType::RANGE);
  dStart.getValue = []() { return String(config.drink.startHour); };
  dStart.onAction = []() {
    config.drink.startHour = (config.drink.startHour + 1) % 24;
    saveConfig();
  };
  menuItems.push_back(dStart);

  // Drink End Hour
  MenuItem dEnd(L(MSG_DRINK_END), MenuItemType::RANGE);
  dEnd.getValue = []() { return String(config.drink.endHour); };
  dEnd.onAction = []() {
    config.drink.endHour = (config.drink.endHour + 1) % 24;
    saveConfig();
  };
  menuItems.push_back(dEnd);

  // Drink Interval
  MenuItem dInt(L(MSG_DRINK_INTERVAL), MenuItemType::RANGE);
  dInt.getValue = []() { return String(config.drink.intervalMinutes) + "m"; };
  dInt.onAction = []() {
    int mins = config.drink.intervalMinutes;
    if (mins < 30) mins = 30;
    else if (mins < 45) mins = 45;
    else if (mins < 60) mins = 60;
    else if (mins < 90) mins = 90;
    else if (mins < 120) mins = 120;
    else mins = 30;
    config.drink.intervalMinutes = mins;
    saveConfig();
  };
  menuItems.push_back(dInt);

  // Drink Goal
  MenuItem dGoal(L(MSG_DRINK_GOAL), MenuItemType::RANGE);
  dGoal.getValue = []() { return String(config.drink.dailyGoalLiters, 1) + "L"; };
  dGoal.onAction = []() {
    float goal = config.drink.dailyGoalLiters;
    goal += 0.5;
    if (goal > 4.0) goal = 1.0;
    config.drink.dailyGoalLiters = goal;
    saveConfig();
  };
  menuItems.push_back(dGoal);

  // Clear Missed
  MenuItem dClear(L(MSG_CLEAR_MISSED), MenuItemType::ACTION);
  dClear.getValue = []() { return String(getMissedReminders()); };
  dClear.onAction = []() {
    resetMissedReminders();
  };
  menuItems.push_back(dClear);
}

void initMenu() {
  setupMenuItems();
  selectedIndex = 0;
  initialized = true;
}

void loopSettings() {
  if (!initialized) initMenu();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf); // Use a smaller, clear font

  int itemHeight = 12;
  int visibleCount = 64 / itemHeight; // 64 / 12 = 5 items
  int scrollOffset = 0;

  if (selectedIndex >= visibleCount) {
    scrollOffset = selectedIndex - visibleCount + 1;
  }

  // Draw scrollbar
  int scrollBarHeight = 64;
  int barSize = (visibleCount * scrollBarHeight) / menuItems.size();
  if (barSize < 4) barSize = 4;
  int barPos = (selectedIndex * (scrollBarHeight - barSize)) / (menuItems.size() - 1);
  u8g2.drawVLine(126, 0, scrollBarHeight);
  u8g2.drawBox(125, barPos, 3, barSize);

  for (int i = 0; i < visibleCount; i++) {
    int idx = i + scrollOffset;
    if (idx >= menuItems.size()) break;

    int y = (i + 1) * itemHeight - 2;
    
    if (idx == selectedIndex) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, i * itemHeight, 124, itemHeight);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }

    u8g2.drawUTF8(2, y, menuItems[idx].label.c_str());

    if (menuItems[idx].getValue) {
      String val = menuItems[idx].getValue();
      int valWidth = u8g2.getStrWidth(val.c_str());
      u8g2.drawUTF8(122 - valWidth, y, val.c_str());
    }
  }
  u8g2.setDrawColor(1);
  sendBuffer();
}

void handleMenuClick() {
  selectedIndex = (selectedIndex + 1) % menuItems.size();
}

void handleMenuDoubleClick() {
  if (selectedIndex < menuItems.size() && menuItems[selectedIndex].onAction) {
    menuItems[selectedIndex].onAction();
  }
}

void handleMenuTripleClick() { Router::pop(); }
