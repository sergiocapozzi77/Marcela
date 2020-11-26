#include "fsHelper.h"

#include "SPIFFS.h"
#include "SD.h"
#define FORMAT_SPIFFS_IF_FAILED true
File indexFile;

void deleteIfExists(fs::FS &fs, const char * path)
{
    Serial.printf("deleting: %s\r\n", path);
    if(fs.exists(path))
    {
        Serial.printf("deleted: %s\r\n", path);
        fs.remove(path);
    }
}

void spiffsSetup()
{
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }    
}

bool SDSetup()
{
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return false;
    }

    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return false;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);   

    return true; 
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void writeFile(fs::FS &fs, const char * path, const char * message, const int len){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.write((uint8_t *) message, len)){
        Serial.println("- file written");
    } else {
        Serial.println("- frite failed");
    }

    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message, const int len){
    //Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.write((uint8_t *) message, len)){
    //    Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }

    file.close();
}

bool startReadingIndex(fs::FS &fs)
{
     StaticJsonDocument<200> *indexDoc = new StaticJsonDocument<200>();
      indexFile = fs.open("/index.txt");
      if(!indexFile || indexFile.isDirectory()){
        Serial.println("- failed to open file for reading");
        return false;
    }

    return true;
}

void endReadingIndex()
{
    indexFile.close();
}

void readNextIndexConfig(StaticJsonDocument<200> &indexDoc)
{
    if(indexFile.available())
    {
        String line = indexFile.readStringUntil('\n');
        Serial.print("Reading: ");
        Serial.println(line);
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(indexDoc, line.c_str());

        // Test if parsing succeeds.
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        return;
    }
    
    return;
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){        
        Serial.println(file.readStringUntil('\n'));
        Serial.println("--------");
    }
}