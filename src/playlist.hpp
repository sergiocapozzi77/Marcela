#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <Arduino.h>

class Playlist
{
    String downloadIndex(const char *name);
    bool readPlayList(String content);

public:
    bool loadPlaylist(const char *name);
};

extern Playlist playlist;

#endif