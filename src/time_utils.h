#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>

bool syncNTP();
void syncLocalFromChronos();
int getHour24();

#endif