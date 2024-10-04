#include "player.h"

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWav.h"
#include "AudioOutputI2S.h"
#include "fsHelper.h"
#include "common.h"
#include "EmmaSleep.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"

AudioGeneratorMP3 *mp3;
AudioGeneratorWAV *wav;
AudioFileSourceSD *fileToPlay;
AudioOutputI2S *out;
AudioGenerator *audio = NULL;

AudioFileSourceHTTPStream *httpStream;
AudioFileSourceBuffer *buff;

// Digital I/O used
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

int mp3FilesCount;
String mp3Files[255];

void setupPlayer()
{
    httpStream = new AudioFileSourceHTTPStream("https://raw.githubusercontent.com/sergiocapozzi77/Marcela/master/content/0000-1111/EarthWindFire.mp3");
    // httpStream = new AudioFileSourceHTTPStream("http://streamingv2.shoutcast.com/radiofreccia");
    buff = new AudioFileSourceBuffer(httpStream, 4096);

    fileToPlay = new AudioFileSourceSD();
    out = new AudioOutputI2S();
    mp3 = new AudioGeneratorMP3();
    wav = new AudioGeneratorWAV();

    randomSeed(2343);

    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    refreshDirContent();
}

void refreshDirContent()
{
    getDirContent(activeFS, "/mp3", mp3FilesCount, mp3Files);
}

void playRandomEffect()
{
    if (mp3FilesCount == 0)
    {
        Serial.println("no effects");
        return;
    }

    String fileToPlay = mp3Files[random(mp3FilesCount)];
    Serial.print("Playing effect: ");
    Serial.println(fileToPlay);
    playFile(fileToPlay);
}

void playStream()
{
    audio = mp3;
    Serial.println("Playing stream");
    mp3->begin(buff, out);
}

void playFile(String fileName)
{
    Serial.println("Stop player");
    if (mp3->isRunning())
        mp3->stop();
    if (wav->isRunning())
        wav->stop();

    String extension = getExtension(fileName);
    extension.toUpperCase();

    if (extension == ".MP3")
    {
        audio = mp3;
    }
    else if (extension == ".WAV")
    {
        audio = wav;
    }
    else
    {
        Serial.print("Extension not recognised ");
        Serial.println(extension);
        audio = NULL;
    }

    Serial.println("Open file");
    if (!fileToPlay->open(fileName.c_str()))
    {
        Serial.println("*** Error file open");
    }
    Serial.println("Begin play");

    if (!audio->begin(fileToPlay, out))
    {
        Serial.println("*** Error Begin play");
    }
}

bool loopPlayer()
{
    if (audio == NULL)
    {
        // Serial.println("audio null");
        return false;
    }

    if (audio->isRunning())
    {
        if (!audio->loop())
        {
            audio->stop();
            //  Serial.println("stopping");
        }

        resetSleep();
        return true;
    }
    else
    {
        //  Serial.println("finish");
    }

    return false;
    /*  else
      {
          Serial.printf("MP3 done\n");
          delay(1000);
      }*/
}