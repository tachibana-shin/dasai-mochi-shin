#include "dasai_mochi.h"

#include <vector>

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

  if (qgifs.size() == 0) {
    Serial.println("No GIF files found");
    showMessage("No GIF files found", 3000);
    return;
  }

  allQGifs = qgifs;
}

void nextDasaiMochi() {
  resetFrame();
  gifPlayerSetFile(config.homePath + "/DasaiMochi/Mochi/" +
                   allQGifs[random(0, allQGifs.size())]);
}

void loopDasaiMochi() {
  gifPlayerTick();

  if (isEndGif()) {
    nextDasaiMochi();
  }
}