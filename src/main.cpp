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

#include "player.h"
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

WiFiMulti wifiMulti;
const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
const char* password = "hszdtubp";

fs::FS activeFS = SD;
#define BASE_ADDRESS "https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/"

unsigned int currentVersion = 0;
unsigned int cycles = 0;

const uint64_t uS_TO_S_FACTOR = 1000000;  /* Conversion factor for micro seconds to seconds */
const uint64_t TIME_TO_SLEEP = 3600;        /* Time ESP32 will go to sleep (in seconds) */



void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d %H:%M:%S", &timeinfo);
  
  NVS.setString("time", timeStringBuff);
  Serial.print("***** ");
  Serial.println(timeStringBuff);
}

void manageOTA(uint32_t version, String link)
{
    Serial.println("Found OTA");
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
    Serial.println("Found mp3");
    if(!doc.containsKey("target"))
    {
        Serial.println("mp3 config has no target");
        return;
    }

    if(downloadFile(link.c_str(), doc["target"].as<String>(), activeFS, false))
    {
        currentVersion = version;
        //NVS.setInt("version", version);
        Serial.print("Writing version: ");
        Serial.println(version);
        Serial.println("mp3 downloaded");
    }
}

void downloadIndex()
{
    String indexLink = "index";
    indexLink = BASE_ADDRESS + indexLink;
    downloadFile(indexLink.c_str(), "/index.txt", activeFS, false);
    listDir(activeFS, "/", 0);   
    readFile(activeFS, "/index.txt");
}

void updateAll()
{
    listDir(activeFS, "/", 0); 

    downloadIndex();

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
                    currentVersion = version;
                    manage_mp3(version, link, doc);
                    play("/mp3/jbells.mp3");
                }
            }
            else
            {
                Serial.println("doc is null");
            }
        
        } while(!doc.isNull());

        Serial.println("End reading");

        endReadingIndex();
    }

    listDir(activeFS, "/mp3", 0);
}

void setup() {
    Serial.begin(115200);

    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * TIME_TO_SLEEP);

    pinMode (2, OUTPUT);

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

    String tm = NVS.getString("time");
    Serial.print("*** Time read: ");
    Serial.println(tm);

    spiffsSetup();
    currentVersion = NVS.getInt("version");

    Serial.print("Current version: ");Serial.println(currentVersion);
    listDir(activeFS, "/", 0);
    listDir(activeFS, "/mp3", 0);

    Serial.println();
    Serial.println();
    Serial.println();

    wifiMulti.addAP(ssid, password);

    Serial.println("Setting up player");
    setupPlayer();

    while(wifiMulti.run() != WL_CONNECTED) {
        Serial.println("waiting for connection");
        delay(1000);
    }

    digitalWrite (2, HIGH);
    updateAll();
    digitalWrite (2, LOW);
    //Serial.println("Singing");
    play("/mp3/LastChristmas.mp3");
}

void loop() {
    loopPlayer();

/*
    
    if((wifiMulti.run() == WL_CONNECTED)) {
       

        //init and get the time
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        printLocalTime();

        Serial.println("Going to sleep");
        digitalWrite (2, LOW);
        delay(1000);
        Serial.flush(); 

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        
        
      //  esp_deep_sleep_start();

    }*/
}