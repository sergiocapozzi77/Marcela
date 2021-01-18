
#ifndef EMMABUTTON_H
#define EMMABUTTON_H

//callback  para envendos das características
class EmmaButton {
    const int VALUE_THRESHOLD = 70;
    bool buttonPressed = false;
    bool lastButtonPressed = false;
    // the following variables are unsigned longs because the time, measured in
    // milliseconds, will quickly become a bigger number than can be stored in an int.
    unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
    unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
    int buttonId;
    public:
        EmmaButton(int buttonId);
        bool isPressed();
};

#endif
