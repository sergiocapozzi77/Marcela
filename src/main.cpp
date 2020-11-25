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
#include "SPIFFS.h"

WiFiMulti wifiMulti;
const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
const char* password = "hszdtubp";


void setup() {

    Serial.begin(115200);

    spiffsSetup();

    listDir(SPIFFS, "/", 0);

    Serial.println();
    Serial.println();
    Serial.println();
    readFile(SPIFFS, "/index.txt");

    wifiMulti.addAP(ssid, password);
}

void loop() {
    if((wifiMulti.run() == WL_CONNECTED)) {
        downloadFile("https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/index", "/index.txt");
        //downloadFile("https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/0011", "/0011.mp3");
    }
       
    listDir(SPIFFS, "/", 0);   
    readFile(SPIFFS, "/index.txt");
    if(startReadingIndex())
    {
        Serial.println("Found lines");
        StaticJsonDocument<200> doc;
        do {
            doc = readNextIndexConfig();
            if(doc.isNull())
            {
                if(doc.containsKey("type"))
                {
                    Serial.print("Found type: ");
                    String type = doc["sd"].as<String>();
                    //Serial.println(operator["type"].c_str);
                }
            }
            else
            {
                Serial.println("doc is null");
            }
        
            //delete doc;
        } while(doc.isNull());
        Serial.println("End reading");

        endReadingIndex();
    }
    delay(50000);
}