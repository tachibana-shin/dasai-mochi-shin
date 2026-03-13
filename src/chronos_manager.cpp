#include "chronos_manager.h"

#include <ChronosESP32.h>

#include "config.h"
#include "time_utils.h"

ChronosESP32 chronos;

void initChronos() {
  chronos.setName(config.bluetoothName);
  chronos.setConnectionCallback([](bool enabled) {
    if (enabled) syncLocalFromChronos();
  });
  chronos.setNotificationCallback([](Notification notify) {
    // notification handling (optional)
  });
  chronos.setConfigurationCallback(
      [](Config _, uint32_t timestamp, uint32_t timezone) {
        bool is24Hour = chronos.is24Hour();
        if (is24Hour != config.is24Hour) {
          config.is24Hour = is24Hour;
          saveConfig();
        }
      });

  if (config.bluetoothEnabled) {
    chronos.begin();
  }
}

void loopChronos() {
  if (config.bluetoothEnabled) {
    chronos.loop();
  }
}