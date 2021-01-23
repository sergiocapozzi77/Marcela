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

#include "player.h"
#include "time.h"
#include "common.h"
#include "ContentManager.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

//const char* ssid = "99BB Hyperoptic 1Gbps Broadband";
//const char* password = "hszdtubp";

const uint64_t uS_TO_S_FACTOR = 1000000;  /* Conversion factor for micro seconds to seconds */
const uint64_t TIME_TO_SLEEP = 43200;   /* Time ESP32 will go to sleep (in seconds) */

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

void setupWifi()
{
  WiFi.mode(WIFI_AP_STA); // explicitly set mode, esp defaults to STA+AP

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

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
}

void setup() {
  Serial.begin(115200);

  //goToDeepSleep();

  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * TIME_TO_SLEEP);

  pinMode (2, OUTPUT);

  if(!SDSetup())
  {
      Serial.println("Unable to read SD");
      delay(3000);
      ESP.restart();
  }

  listDir(activeFS, "/", 0);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
  {
    if(!NVS.begin())
    {
        Serial.println("Unable to initialize NVS");
        delay(3000);
        ESP.restart();
      //  return;
    }

    String tm = NVS.getString("time");
    Serial.print("*** Time read: ");
    Serial.println(tm);
    Serial.println();    setupWifi();
    startContentManager();
    //resetVersion();
    digitalWrite (2, HIGH);
    updateAll();
    digitalWrite (2, LOW);

    goToDeepSleep();
  }

  setupSleep();
  setupPlayer();

  playFile("/mp3/0011.WAV");
  Serial.println("------------------------------- Setup finished ------------------------------- ");
}

void loop() {
    bool isPlaying = loopPlayer();
    if(but0->isPressed())
    {
      playRandomEffect();
    }

    checkSleep();
}