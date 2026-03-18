#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern U8G2 *u8g2;
extern bool screenOn;
extern SemaphoreHandle_t displayMutex;

enum {
  SHOW_TRUNCATE = 0,
  SHOW_WRAP,
  SHOW_MARQUEE
};

// Cached UI units (using float for precision)
extern float _vw_u, _vh_unit;
extern int _rem_u;

void initDisplay();
void refreshUIUnits();
void toggleScreen();
void showMessage(const char* msg, uint32_t timeout = 1000, uint8_t mode = SHOW_WRAP);
void loopDisplay();
void sendBuffer();
bool isShowingMessage();
void drawMessageContent();
void clearMessage();
void resetScreenTimer();

// Precise responsive helpers
inline int vw(float p) { return (int)(p * _vw_u); }
inline int vh(float p) { return (int)(p * _vh_unit); }
inline int rem(float s) { return (int)(s * _rem_u); }

#endif