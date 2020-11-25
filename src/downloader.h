#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <Arduino.h>
#include "FS.h"

void downloadFile(const char *link, String fileName, fs::FS &fs, bool isOta);

#endif