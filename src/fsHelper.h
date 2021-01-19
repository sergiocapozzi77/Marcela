#ifndef FSHELPER_H
#define FSHELPER_H

#include "FS.h"
#include <Arduino.h>
#include <ArduinoJson.h>

bool SDSetup();
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
bool writeFile(fs::FS &fs, const char * path, const char * message, const int len);
bool appendFile(File file, const char * message, const int len);
void readFile(fs::FS &fs, const char * path);
void readNextIndexConfig(StaticJsonDocument<200> &indexDoc);
bool startReadingIndex(fs::FS &fs);
void endReadingIndex();
bool deleteIfExists(fs::FS &fs, const char * path);
File openFile(fs::FS &fs, const char * path);
void closeFile(File file);
void getDirContent(fs::FS &fs, const char * dirname, int &count, String* files);
String getExtension(String fileName);

#endif