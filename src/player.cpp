#include "player.h"

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWav.h"
#include "AudioOutputI2SNoDAC.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *fileToPlay;
AudioOutputI2SNoDAC *out;

void setupPlayer()
{
    fileToPlay = new AudioFileSourceSD();
    out = new AudioOutputI2SNoDAC();
    mp3 = new AudioGeneratorMP3();
}

void play(const char *fileName)
{
    Serial.println("Stop player");
    if (mp3->isRunning())
        mp3->stop();

    Serial.println("Open file");
    if (!fileToPlay->open(fileName))
    {
        Serial.println("*** Error file open");
    }
    Serial.println("Begin play");
    if (!mp3->begin(fileToPlay, out))
    {
        Serial.println("*** Error Begin play");
    }
}

void loopPlayer()
{
    if (mp3->isRunning())
    {
        if (!mp3->loop())
            mp3->stop();
    }
    else
    {
        Serial.printf("MP3 done\n");
        delay(1000);
    }
}