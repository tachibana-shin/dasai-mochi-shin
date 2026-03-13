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

#endif  // GIF_PLAYER_H