#include "player2.h"

#include "Audio.h"
#include "fsHelper.h"
#include "common.h"

Audio audio2;

// Digital I/O used
#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

int mp3FilesCount2;
String mp3Files2[255];

void setupPlayer2()
{
    randomSeed(2343);

    audio2.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio2.setVolume(20);

    refreshDirContent2();
}

void refreshDirContent2()
{
    getDirContent(activeFS, "/mp3", mp3FilesCount2, mp3Files2);
}

void playRandomEffect2()
{
  if(mp3FilesCount2 == 0)
  {
    Serial.println("no effects");    
    return;
  }

  String fileToPlay = mp3Files2[random(mp3FilesCount2)];
  Serial.print("Playing effect: ");
  Serial.println(fileToPlay);
  playFile2(fileToPlay);
}

void playFile2(String fileName)
{
    Serial.println("Stop player");
    if (audio2.isRunning())
        audio2.stopSong();


    String extension = getExtension(fileName);
    extension.toUpperCase();

/*    if(extension == ".MP3")
    {
        audio = mp3;
    }
    else if(extension == ".WAV")
    {
        audio = wav;
    }
    else
    {
        Serial.print("Extension not recognised ");
        Serial.println(extension);
        audio = NULL;
    }*/

    Serial.println("Open file");
    if (!audio2.connecttoFS(activeFS, fileName))
    {
        Serial.println("*** Error file open");
    }

}

bool loopPlayer2()
{
    audio2.loop();
}