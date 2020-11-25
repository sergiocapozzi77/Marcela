#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <Arduino.h>

void downloadFile(const char *link, String fileName, bool isOta);

#endif