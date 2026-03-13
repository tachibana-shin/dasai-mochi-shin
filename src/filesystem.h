#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <SD.h>
#include <SPIFFS.h>

extern bool sdInitialized;
extern SPIClass sdSPI;

void initSD();
fs::File getFile(const String& path, const char* mode, bool useSD = true);

#endif
