#include "filesystem.h"

#include <LittleFS.h>
#include <SD.h>

#include "config.h"

bool sdInitialized = false;
SPIClass sdSPI(HSPI);

void initSD() {
  if (sdInitialized) return;

  sdSPI.begin(config.pinSdCLK, config.pinSdMISO, config.pinSdMOSI,
              config.pinSdCS);
  if (!SD.begin(config.pinSdCS, sdSPI)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");
  sdInitialized = true;
}

fs::File getFile(const String& path, const char* mode, bool useSD) {
  fs::File file;

  if (useSD && sdInitialized) {
    file = SD.open(path, mode);
    if (!file) {
      Serial.println("Failed to open " + path + " on SD with mode " + mode);
      return fs::File();   // 空の File を返す
    }
    return file;
  }

  // SD を使わない、または SD が未初期化 → LittleFS を使う
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return fs::File();
  }

  file = LittleFS.open(path, mode);
  if (!file) {
    Serial.println("Failed to open " + path + " on LittleFS with mode " + mode);
    return fs::File();
  }

  return file;
}