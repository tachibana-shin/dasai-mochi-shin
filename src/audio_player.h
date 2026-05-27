#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include <FS.h>

enum SoundType { SOUND_CLICK, SOUND_NOTIFY };

void audioInit();
void audioPlayFile(const fs::File& file);
void audioPlayDefault(SoundType type);
void audioStop();
bool isAudioPlaying();

#endif
