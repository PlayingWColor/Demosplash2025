#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef INCLUDE_SDL
#define INCLUDE_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif

#include "vulkan.h"
#include "music.h"
#include "cglm/cglm.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define TITLE "Playing with Color - Demosplash 2025"

const char* musicFile = "../assets/music.wav";

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	//Load Music
	LoadMusic(musicFile);

	//Create Window for Vulkan Usage
	
	SDL_Window* window;
	
	bool shouldClose = false;

	window = SDL_CreateWindow(
		TITLE, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN
	);
	
	if (window == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return 1;
	}
	
	InitializeVulkan(TITLE, window);
	
	BeginPlayMusic();

	//Main Loop
	while(!shouldClose) {
		if(SDL_GetTicks() > 5000)
			FrameUpdateMusic();

		SDL_Event event;
		
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_ESCAPE)) {
				shouldClose = true;
			}
		}
		DrawFrame();

	}
	
	//Close Application
	CloseMusic();

	CleanUpVulkan();

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
