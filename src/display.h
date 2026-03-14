#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern bool screenOn;

enum {
  SHOW_TRUNCATE = 0,
  SHOW_WRAP,
  SHOW_MARQUEE
};

void initDisplay();
void toggleScreen();
void showMessage(const char* msg, uint32_t timeout = 1000, uint8_t mode = SHOW_WRAP);
void loopDisplay();
void sendBuffer();

#endif