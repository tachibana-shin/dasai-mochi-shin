#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>

void initAlarm();
void loopAlarm();
bool isAlarmActive();
void stopAlarm();

#endif
