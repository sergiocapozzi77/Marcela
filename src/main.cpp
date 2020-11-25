/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include "downloader.h"
#include "fsHelper.h"
#include "SD.h"
#include "SPIFFS.h"
#include "EEPROM.h"

int eepromAddr = 0;
#define EEPROM_SIZE 64

WiFiMulti wifiMulti;
const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
const char* password = "hszdtubp";

fs::FS activeFS = SPIFFS;
#define BASE_ADDRESS "https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/"

unsigned int currentVersion = 0;

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeUnsignedIntIntoEEPROM(int address, unsigned int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

unsigned int readUnsignedIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeUnsignedLongIntoEEPROM(int address, unsigned long number)
{ 
  EEPROM.write(address, (number >> 24) & 0xFF);
  EEPROM.write(address + 1, (number >> 16) & 0xFF);
  EEPROM.write(address + 2, (number >> 8) & 0xFF);
  EEPROM.write(address + 3, number & 0xFF);
}
unsigned long readUnsignedLongFromEEPROM(int address)
{
  return ((unsigned long)EEPROM.read(address) << 24) +
         ((unsigned long)EEPROM.read(address + 1) << 16) +
         ((unsigned long)EEPROM.read(address + 2) << 8) +
         (unsigned long)EEPROM.read(address + 3);
}

void setup() {

    Serial.begin(115200);
    if(!SDSetup())
    {
        Serial.println("Unable to read SD");
      //  return;
    }

    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("failed to initialise EEPROM");
    }

    unsigned long magicKey = ESP.getEfuseMac();
    unsigned long readKey = readUnsignedLongFromEEPROM(60);
    if(readKey != magicKey)
    { // first startup
        Serial.print("Different magic keys: first startup");
        writeUnsignedLongIntoEEPROM(60, magicKey);
        currentVersion = 0;
        writeUnsignedIntIntoEEPROM(0, currentVersion);
    }

    spiffsSetup();
    currentVersion = readUnsignedIntFromEEPROM(0);

    Serial.print("Current version: ");Serial.println(currentVersion);
    listDir(activeFS, "/", 0);

    Serial.println();
    Serial.println();
    Serial.println();

    wifiMulti.addAP(ssid, password);
}

void loop() {
    if((wifiMulti.run() == WL_CONNECTED)) {
        listDir(activeFS, "/", 0); 

        downloadFile("https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/index", "/index.txt", activeFS, false);

        listDir(activeFS, "/", 0);   
        readFile(activeFS, "/index.txt");
        if(startReadingIndex(activeFS))
        {
            Serial.println("Found lines");
            StaticJsonDocument<200> doc;
            do {
                readNextIndexConfig(doc);
                if(!doc.isNull())
                {
                    unsigned int version = doc["id"].as<unsigned int>();
                    Serial.print("Read version: ");Serial.println(version);
                    if(version <= currentVersion)
                    {
                        Serial.print("Skipping version: ");Serial.println(version);
                        continue;
                    }

                    if( strcmp(doc["type"], "ota") == 0)
                    {
                        Serial.print("Found OTA: ");
                        String link = doc["link"].as<String>();
                        link = BASE_ADDRESS + link;
                        Serial.print("Link: ");
                        Serial.println(link);
                        if(downloadFile(link.c_str(), "", activeFS, true))
                        {
                            currentVersion = version;
                            Serial.print("Writing version: ");
                            Serial.println(version);
                            writeUnsignedIntIntoEEPROM(0, version);
                            ESP.restart();
                        }
                    }
                }
                else
                {
                    Serial.println("doc is null");
                }
            
                //delete doc;
            } while(!doc.isNull());

            Serial.println("End reading");

            endReadingIndex();
        }
        delay(500000);
    }
}