#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>

enum SoundType { SOUND_CLICK, SOUND_DRINK, SOUND_ALARM, SOUND_NOTIFY };

void audioInit();
void audioPlayDefault(SoundType type);
void audioStop();
bool isAudioPlaying();

#endif
