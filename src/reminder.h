#ifndef REMINDER_H
#define REMINDER_H

#include <Arduino.h>

void initReminder();
void loopReminder();
void recordDrinkEvent(String type, float amount = 0);
bool isReminderActive();
void confirmDrink();
int getMissedReminders();
void resetMissedReminders();

#endif
