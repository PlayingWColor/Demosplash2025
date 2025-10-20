#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef INCLUDE_SDL
#define INCLUDE_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif

#include "vulkan.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define TITLE "Playing with Color - Demosplash 2025"

int main(int argc, char* argv[])
{
	//Create Window for Vulkan Usage
	
	SDL_Window* window;
	SDL_Init(SDL_INIT_VIDEO);
	
	bool shouldClose = false;

	window = SDL_CreateWindow(
		TITLE, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN
	);
	
	if (window == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
		return 1;
	}
	
	InitializeVulkan(TITLE, window);

	//Main Loop

	while(!shouldClose) {
		SDL_Event event;
		
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_ESCAPE)) {
				shouldClose = true;
			}
			DrawFrame();
		}

	}
	
	//Close Application
	
	CleanUpVulkan();

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
