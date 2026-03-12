#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

extern bool isPortalActive;

void initWiFi();
void connectWiFi();
bool openWiFiManager();

#endif