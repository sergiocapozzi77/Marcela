#include "playlist.hpp"
#include "fsHelper.h"
#include "SD.h"
#include "downloader.h"
#include "common.h"
#include <ArduinoJson.h>

Playlist playlist;

bool Playlist::loadPlaylist(const char *name)
{
    String indexFileName = downloadIndex(name);
    if (indexFileName == "")
    {
        return false;
    }

    String content = readAllFile(activeFS, indexFileName.c_str());

    readPlayList(content);

    return true;
}

bool Playlist::readPlayList(String content)
{
    // Allocate the JSON document
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, content.c_str());

    // Test if parsing succeeds
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return false;
    }

    JsonArray items = doc.as<JsonArray>();
    for (int i = 0; i < items.size(); i++)
    {
        String link = items[i]["link"].as<String>();
        String localPath = items[i]["target"].as<String>();
        downloadFile2((BASE_ADDRESS + link).c_str(), localPath, activeFS, false);
    }

    return true;
}

String Playlist::downloadIndex(const char *name)
{
    String indexLink = "/index.json";
    indexLink = BASE_ADDRESS + String(name) + indexLink;
    String fileName = "/" + String(name) + "/index.json";

    if (fileExists(activeFS, fileName.c_str()))
    {
        Serial.println("Index exist, skip download");
        return fileName;
    }

    if (!downloadFile2(indexLink.c_str(), fileName, activeFS, false))
    {
        return "";
    }

    Serial.println("Reading index file");
    listDir(activeFS, ("/" + String(name)).c_str(), 0);
    readAllFile(activeFS, fileName.c_str());

    return fileName;
}