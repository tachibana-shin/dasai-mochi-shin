#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

extern int onlineTemp;
extern int onlineHumidity;
extern float onlineWindSpeed;
extern int onlineWeatherCode;
extern bool onlineIsDay;
extern String onlineSunrise;
extern String onlineSunset;
extern volatile bool weatherUpdating;

void loopWeather();

#endif