#include "EmmaSleep.h"

#include <Arduino.h>

unsigned long lastInteraction;
const long maxAwakeTime = 60000; //3 minutes
//const long maxAwakeTime = 20000; //20 seconds

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

/*
Method to print the touchpad by which ESP32
has been awaken from sleep
*/
void print_wakeup_touchpad(){
  touch_pad_t touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch(touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}

void callback(){
  //placeholder callback function
}

void setupSleep()
{
    print_wakeup_reason();

    touchAttachInterrupt(T0, callback, 70);

    esp_sleep_enable_touchpad_wakeup();

    lastInteraction = millis();
}

void resetSleep()
{
    lastInteraction = millis();
    Serial.println("Sleep has been reset");
}

void checkSleep()
{
    if(millis() - lastInteraction > maxAwakeTime)
    {
        //clearDisplay();
        Serial.println("Going to sleep");
      //  Serial.print("Player is: ");
      //  Serial.println(myDFPlayer.readState());
      //  myDFPlayer.sleep();
        esp_deep_sleep_start();
    }
}