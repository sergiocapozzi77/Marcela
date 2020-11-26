/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include <ArduinoNvs.h> 
#include <WiFi.h>
#include <WiFiMulti.h>

#include "downloader.h"
#include "fsHelper.h"
#include "SD.h"
#include "SPIFFS.h"

WiFiMulti wifiMulti;
const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
const char* password = "hszdtubp";

fs::FS activeFS = SD;
#define BASE_ADDRESS "https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/"

unsigned int currentVersion = 0;


void setup() {

    Serial.begin(115200);
    if(!SDSetup())
    {
        Serial.println("Unable to read SD");
        delay(3000);
        ESP.restart();
    }

    if(!NVS.begin())
    {
        Serial.println("Unable to initialize NVS");
        delay(3000);
        ESP.restart();
      //  return;
    }

    uint64_t magicKey = ESP.getEfuseMac();
    uint64_t readKey = NVS.getInt("mac");
    if(readKey != magicKey)
    { // first startup
        Serial.println("Different magic keys: first startup");
        NVS.setInt("mac", magicKey);
        currentVersion = 0;
        NVS.setInt("version", currentVersion);
    }

    spiffsSetup();
    currentVersion = NVS.getInt("version");

    Serial.print("Current version: ");Serial.println(currentVersion);
    listDir(activeFS, "/", 0);

    Serial.println();
    Serial.println();
    Serial.println();

    wifiMulti.addAP(ssid, password);
}

void manageOTA(uint32_t version, String link)
{
    Serial.print("Found OTA");
    if(downloadFile(link.c_str(), "", activeFS, true))
    {
        currentVersion = version;
        Serial.print("Writing version: ");
        Serial.println(version);
        NVS.setInt("version", version);
        ESP.restart();
    }
}

void manage_mp3(uint32_t version, String link, JsonDocument &doc)
{
    Serial.print("Found mp3");
    if(!doc.containsKey("target"))
    {
        Serial.println("mp3 config has no target");
        return;
    }

    if(downloadFile(link.c_str(), doc["target"].as<String>(), activeFS, false))
    {
        currentVersion = version;
        Serial.print("Writing version: ");
        Serial.println(version);
        NVS.setInt("version", version);
        Serial.println("mp3 downloaded");
    }
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
                    uint32_t version = doc["id"].as<uint32_t>();
                    Serial.print("Read version: ");Serial.println(version);
                    if(version <= currentVersion)
                    {
                        Serial.print("Skipping version: ");Serial.println(version);
                        continue;
                    }

                    String link = doc["link"].as<String>();
                    link = BASE_ADDRESS + link;
                    Serial.print("Link: ");
                    Serial.println(link);
                    if( strcmp(doc["type"], "ota") == 0)
                    {
                        manageOTA(version, link);
                    }
                    else if( strcmp(doc["type"], "mp3") == 0)
                    {
                        manage_mp3(version, link, doc);
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