#ifndef GIF_PLAYER_H
#define GIF_PLAYER_H

#include <Arduino.h>
#include <U8g2lib.h>

#include "gif_types.h"

// Request a file change (takes effect on next tick).
// Pass an empty string to stop playback.
void gifPlayerSetFile(const String &filename);

// Return the filename currently being played (empty if idle).
String gifPlayerGetCurrentFile();

// Non-blocking tick -- call from loop().
// Renders the next frame when timing is due.
void gifPlayerTick();

// Stop playback and free memory.
void gifPlayerStop();

// Check if the GIF has finished playing (only valid when playing).
bool isEndGif();

// Reset frame to first frame
void resetFrame();

#endif  // GIF_PLAYER_H