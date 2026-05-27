#include "filesystem.h"

#include <SD.h>
#include <LittleFS.h>

#include <vector>

#include "config.h"

bool sdInitialized = false;
SPIClass sdSPI(FSPI);

void initFFS() {
  // SD を使わない、または SD が未初期化 → LittleFS を使う
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }
}

void initFilesystem() {
  if (sdInitialized) return;

  sdSPI.begin(config.pinSdCLK, config.pinSdMISO, config.pinSdMOSI,
              config.pinSdCS);
  if (!SD.begin(config.pinSdCS, sdSPI)) {
    Serial.println("SD card initialization failed!");
  } else {
    Serial.println("SD card initialized.");
    sdInitialized = true;
  }
  initFFS();

  if (!ensureDirectories(config.homePath)) {
    Serial.println("Failed to create folders for homePath");
    return;
  }

  std::vector<String> dirs = {"/"};
  while (!dirs.empty()) {
    String d = dirs.back(); dirs.pop_back();
    File root = LittleFS.open(d);
    if (!root || !root.isDirectory()) continue;
    File f = root.openNextFile();
    while (f) {
      String name = f.name();
      size_t sz = f.size();
      bool isDir = f.isDirectory();
      if (isDir) { dirs.push_back(name); }
      else { Serial.printf("[littlefs] %s (%u)\n", name.c_str(), sz); }
      f.close();
      f = root.openNextFile();
    }
    root.close();
  }
}

fs::File getFile(const String& path, const char* mode, bool useSD) {
  fs::File file;

  if (useSD && sdInitialized) {
    file = SD.open(path, mode);
    if (!file) {
      Serial.println("Failed to open " + path + " on SD with mode " + mode);
      return fs::File();  // 空の File を返す
    }
    return file;
  }

  file = LittleFS.open(path, mode);
  if (!file) {
    Serial.println("Failed to open " + path + " on LittleFS with mode " + mode);
    return fs::File();
  }

  return file;
}

std::vector<String> readdir(const String& path, bool useSD) {
  std::vector<String> fileList;
  fs::File root;

  if (useSD && sdInitialized) {
    root = SD.open(path);
  } else {
    root = LittleFS.open(path);
  }

  if (!root || !root.isDirectory()) {
    return fileList;
  }

  fs::File entry = root.openNextFile();
  while (entry) {
    String name = String(entry.name());
    fileList.push_back(name);

    entry.close();
    entry = root.openNextFile();
  }

  root.close();
  return fileList;
}

// 親フォルダをすべて作成する関数 (mkdir -p 相当)
bool ensureDirectories(const String& fullPath) {
  String path = fullPath;

  // ファイル名を削除してディレクトリ部分だけにする
  int lastSlash = path.lastIndexOf('/');
  if (lastSlash <= 0) return true;
  path = path.substring(0, lastSlash);

  // スラッシュごとに区切って作成
  String current = "";
  for (int i = 0; i < path.length(); i++) {
    char c = path[i];
    current += c;

    // フォルダ名が確定したタイミングで mkdir
    if (c == '/') {
      if (!LittleFS.exists(current)) {
        // 存在しなければ作成
        // mkdir は途中階層でも OK
        if (!LittleFS.mkdir(current)) {
          Serial.printf("Failed to mkdir: %s\n", current.c_str());
          return false;
        }
      }
    }
  }

  return true;
}