#include "dasai_mochi.h"

#include <vector>

#include "assets/intro.h"
#include "config.h"
#include "display.h"
#include "e_locale.h"
#include "filesystem.h"
#include "gif_player/gif_player.h"

static std::vector<String> allQGifs;
static std::vector<size_t> playbackQueue;

static void refillAndShuffleQueue() {
  playbackQueue.clear();
  if (allQGifs.empty()) return;
  for (size_t i = 0; i < allQGifs.size(); i++) {
    playbackQueue.push_back(i);
  }
  // Fisher-Yates shuffle
  for (int i = playbackQueue.size() - 1; i > 0; i--) {
    int j = random(0, i + 1);
    size_t temp = playbackQueue[i];
    playbackQueue[i] = playbackQueue[j];
    playbackQueue[j] = temp;
  }
}

void initDasaiMochi() {
  // load list file in folder /DasaiMochi/Mochi/*.{qgif+,qgif}

  std::vector<String> qgifs = {};

  for (String filename : readdir(config.homePath + "/Mochi")) {
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
    showMessage(L(MSG_NO_GIFS), 3000);

    // should empty gif then loop gif intro

    gifPlayerPlayMemory(src_assets_intro_qgif, src_assets_intro_qgif_len, true);

    return;
  }

  allQGifs = qgifs;
  refillAndShuffleQueue();
}

void nextDasaiMochi() {
  if (allQGifs.size() == 0) return;

  if (playbackQueue.empty()) {
    refillAndShuffleQueue();
  }

  size_t index = playbackQueue.back();
  playbackQueue.pop_back();

  resetFrame();

  String filename = allQGifs[index];
  Serial.println("Playing GIF: " + filename);
  gifPlayerSetFile(config.homePath + "/Mochi/" + filename, false);
}

void loopDasaiMochi() {
  gifPlayerTick();

  if (isEndGif()) {
    nextDasaiMochi();
  }
}