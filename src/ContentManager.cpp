#include "ContentManager.h"
#include <Arduino.h>
#include "downloader.h"
#include "fsHelper.h"
#include "SD.h"
#include <ArduinoNvs.h> 
#include "common.h"

unsigned int currentVersion = 0;
#define BASE_ADDRESS "https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/"
#define MAX_FAILURES 5

void startContentManager()
{
    uint64_t magicKey = ESP.getEfuseMac();
    uint64_t readKey = NVS.getInt("mac");
    if(readKey != magicKey)
    { // first startup
        Serial.println("Different magic keys: first startup");
        NVS.setInt("mac", magicKey);
        currentVersion = 0;
        NVS.setInt("version", currentVersion);
    }

    currentVersion = NVS.getInt("version");
    Serial.print("Current version: ");Serial.println(currentVersion);
} 

void resetVersion()
{
    currentVersion = 0;
    NVS.setInt("version", currentVersion);
}

bool manageOTA(uint32_t version, String link)
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

    return false;
}

bool manage_mp3(uint32_t version, String link, JsonDocument &doc)
{
    Serial.println("Found mp3");
    if(!doc.containsKey("target"))
    {
        Serial.println("mp3 config has no target");
        return false;
    }

    if(downloadFile(link.c_str(), doc["target"].as<String>(), activeFS, false))
    {
        Serial.println("mp3 downloaded");
        return true;
    }

    return false;
}


bool deleteFile(uint32_t version, JsonDocument &doc)
{
    Serial.println("Found deleteFile");
    if(!doc.containsKey("target"))
    {
        Serial.println("mp3 config has no target");
        return false;
    }

    String target = doc["target"].as<String>();
    deleteIfExists(activeFS, target.c_str());

    return true;
}

bool downloadIndex()
{
    String indexLink = "index";
    indexLink = BASE_ADDRESS + indexLink;
    if(!downloadFile(indexLink.c_str(), "/index.txt", activeFS, false))
    {
        return false;
    }

    listDir(activeFS, "/", 0);   
    readFile(activeFS, "/index.txt");
    
    return true;
}

bool updateAll()
{
    listDir(activeFS, "/", 0); 

    if(!downloadIndex())
    {
        return false;
    }

    int consecutiveFailures = 0;
    
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
                bool result = false;
                if( strcmp(doc["type"], "ota") == 0)
                {
                    result = manageOTA(version, link);
                }
                else if( strcmp(doc["type"], "mp3") == 0)
                {
                    result = manage_mp3(version, link, doc);
                }
                else if( strcmp(doc["type"], "del") == 0)
                {
                    currentVersion = version;
                    result = deleteFile(version, doc);
                }

                if(result)
                {
                    consecutiveFailures = 0;
                    currentVersion = version;
                    NVS.setInt("version", version);
                    Serial.print("Writing version: ");
                    Serial.println(version);
                }
                else
                {
                    consecutiveFailures++;
                    if(consecutiveFailures > MAX_FAILURES)
                    {
                        return false;
                    }
                }
                
            }
            else
            {
                Serial.println("doc is null");
            }

             Serial.print("Next cycle");
        } while(!doc.isNull());

        Serial.println("End reading");

        endReadingIndex();
    }

    return true;
}