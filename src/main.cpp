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

WiFiMulti wifiMulti;
const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
const char* password = "hszdtubp";

fs::FS activeFS = SPIFFS;
#define BASE_ADDRESS "https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/"

void setup() {

    Serial.begin(115200);
    if(!SDSetup())
    {
        Serial.println("Unable to read SD");
      //  return;
    }

    //Serial.println("Ciao OTA");
    spiffsSetup();

    listDir(activeFS, "/", 0);

    Serial.println();
    Serial.println();
    Serial.println();

    wifiMulti.addAP(ssid, password);
}

void loop() {
    if((wifiMulti.run() == WL_CONNECTED)) {
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
                    if( strcmp(doc["type"], "ota") == 0)
                    {
                        Serial.print("Found OTA: ");
                        String link = doc["link"].as<String>();
                        link = BASE_ADDRESS + link;
                        Serial.print("Link: ");
                        Serial.println(link);
                        downloadFile(link.c_str(), "", activeFS, true);
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
        delay(50000);
    }
}