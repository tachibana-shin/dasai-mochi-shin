#include "dasai_mochi.h"

#include <vector>

#include "assets/intro.h"
#include "config.h"
#include "display.h"
#include "filesystem.h"
#include "gif_player/gif_player.h"

static std::vector<String> allQGifs;

void initDasaiMochi() {
  // load list file in folder /DasaiMochi/Mochi/*.{qgif+,qgif}

  std::vector<String> qgifs = {};

  for (String filename : readdir(config.homePath + "/DasaiMochi/Mochi")) {
    if (filename.endsWith(".qgif") || filename.endsWith(".qgif+")) {
      qgifs.push_back(filename);
    }
  }

  // boot image
  gifPlayerPlayMemory(src_assets_intro_qgif, src_assets_intro_qgif_len, false);
  // emulator call loop() to complete boot image;
  while (!isEndGif()) {
    gifPlayerTick();

    delay(10);
  }

  if (qgifs.size() == 0) {
    Serial.println("No GIF files found");
    showMessage("No GIF files found", 3000);

    // should empty gif then loop gif intro

    gifPlayerPlayMemory(src_assets_intro_qgif, src_assets_intro_qgif_len, true);

    return;
  }

  allQGifs = qgifs;
}

void nextDasaiMochi() {
  resetFrame();
  if (allQGifs.size() == 0) return;

  gifPlayerSetFile(config.homePath + "/DasaiMochi/Mochi/" +
                   allQGifs[random(0, allQGifs.size())]);
}

void loopDasaiMochi() {
  gifPlayerTick();

  if (isEndGif()) {
    nextDasaiMochi();
  }
}