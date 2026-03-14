#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>

void initAudio();
void loopAudio();
void playAudio(const char* filename);
void playAlarmAudio();
void playDrinkAudio();
void playNotifyAudio();
void playTapAudio();
void playLockscreenAudio();
void setVolume(uint8_t volume);

#endif
