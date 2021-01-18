#include "player.h"

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWav.h"
#include "AudioOutputI2S.h"
#include "fsHelper.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *fileToPlay;
AudioOutputI2S *out;

// Digital I/O used
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

int mp3FilesCount;
String mp3Files[255];

void setupPlayer()
{
    fileToPlay = new AudioFileSourceSD();
    out = new AudioOutputI2S();
    mp3 = new AudioGeneratorMP3();

    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    getDirContent(SD, "/mp3", mp3FilesCount, mp3Files);
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

bool loopPlayer()
{
    if (mp3->isRunning())
    {
        if (!mp3->loop())
            mp3->stop();

        return true;
    }
    
    return false;
  /*  else
    {
        Serial.printf("MP3 done\n");
        delay(1000);
    }*/
}