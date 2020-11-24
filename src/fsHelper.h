#ifndef FSHELPER_H
#define FSHELPER_H

#include "FS.h"

void spiffsSetup();
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void writeFile(fs::FS &fs, const char * path, const char * message, const int len);
void appendFile(fs::FS &fs, const char * path, const char * message, const int len);
void readFile(fs::FS &fs, const char * path);
void deleteIfExists(fs::FS &fs, const char * path);
#endif