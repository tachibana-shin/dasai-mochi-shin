#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <SD.h>
#include <SPIFFS.h>

#include <vector>

extern bool sdInitialized;
extern SPIClass sdSPI;

void initFilesystem();
fs::File getFile(const String& path, const char* mode, bool useSD = true);
std::vector<String> readdir(const String& path, bool useSD = true);
bool ensureDirectories(const String& fullPath);

#endif
