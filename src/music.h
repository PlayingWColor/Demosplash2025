#ifndef PWC_MUSIC_H
#define PWC_MUSIC_H

#ifndef INCLUDE_SDL
#define INCLUDE_SDL
#include <SDL3/SDL.h>
#endif

void LoadMusic(const char* musicPath);

void BeginPlayMusic();

void FrameUpdateMusic();

void CloseMusic();

#endif
