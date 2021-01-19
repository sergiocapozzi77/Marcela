/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include <ArduinoNvs.h> 
#include <WiFiManager.h>
#include "EmmaButton.h"
#include "EmmaSleep.h"

#include "downloader.h"
#include "fsHelper.h"
#include "SD.h"

#include "player2.h"
#include "time.h"
#include "common.h"
#include "ContentManager.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

//const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
//const char* password = "hszdtubp";

const uint64_t uS_TO_S_FACTOR = 1000000;  /* Conversion factor for micro seconds to seconds */
const uint64_t TIME_TO_SLEEP = 3600;        /* Time ESP32 will go to sleep (in seconds) */

EmmaButton *but0 = new EmmaButton(T0);

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

    startContentManager();
    resetVersion();

    String tm = NVS.getString("time");
    Serial.print("*** Time read: ");
    Serial.println(tm);

    setupSleep();

    listDir(activeFS, "/", 0);
    listDir(activeFS, "/mp3", 0);

    Serial.println();
    Serial.println();
    Serial.println();

    Serial.println("Setting up player");
    setupPlayer2();

    WiFi.mode(WIFI_AP_STA); // explicitly set mode, esp defaults to STA+AP

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("SergioToy"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }

    digitalWrite (2, HIGH);
    updateAll();
    refreshDirContent();
    digitalWrite (2, LOW);
    //Serial.println("Singing");
    playFile2("/mp3/0006.mp3");
    
  //  loopPlayer2();
}

void loop() {
   bool isPlaying = loopPlayer2();
    if(but0->isPressed())
    {
      playRandomEffect2();
    }
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

 //   checkSleep();
}