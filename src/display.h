#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern bool screenOn;

void initDisplay();
void toggleScreen();
void showMessage(const char* msg);
void loopDisplay();
void sendBuffer();

#endif