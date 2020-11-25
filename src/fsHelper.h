#ifndef FSHELPER_H
#define FSHELPER_H

#include "FS.h"
#include <Arduino.h>
#include <ArduinoJson.h>

void spiffsSetup();
bool SDSetup();
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void writeFile(fs::FS &fs, const char * path, const char * message, const int len);
void appendFile(fs::FS &fs, const char * path, const char * message, const int len);
void readFile(fs::FS &fs, const char * path);
void readNextIndexConfig(StaticJsonDocument<200> &indexDoc);
bool startReadingIndex(fs::FS &fs);
void endReadingIndex();
void deleteIfExists(fs::FS &fs, const char * path);
#endif