#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>

void setupPlayer();
void playFile(String fileName);
bool loopPlayer();
void playRandomEffect();

#endif