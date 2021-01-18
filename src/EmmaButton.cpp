#include <Arduino.h>
#include "EmmaButton.h"
#include "EmmaSleep.h"

EmmaButton::EmmaButton(int id)
{
    buttonId = id;
}

bool EmmaButton::isPressed()
{
    bool ret = false;
    int touchSensorValue = touchRead(buttonId);
    bool buttonPressedReading;
    if(touchSensorValue < VALUE_THRESHOLD)
    { // pressed
        buttonPressedReading = true;
    }
    else
    { // not pressed
        buttonPressedReading = false;
    }

    // If the switch changed, due to noise or pressing:
    if (buttonPressedReading != lastButtonPressed) {
    // reset the debouncing timer
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

        // if the button state has changed:
        if (buttonPressedReading != buttonPressed) {
            buttonPressed = buttonPressedReading;

            // only toggle the LED if the new button state is HIGH
            if (buttonPressed) {
                Serial.println("Button pressed");
                ret = true;
                resetSleep();
            }
        }
    }

    lastButtonPressed = buttonPressedReading;

    return ret;
}