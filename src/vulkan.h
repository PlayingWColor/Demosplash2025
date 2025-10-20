#ifndef PWC_VULKAN_H
#define PWC_VULKAN_H

#ifndef INCLUDE_SDL
#define INCLUDE_SDL
#include <SDL3/SDL.h>
#endif

#ifndef INCLUDE_VULKAN
#define INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>
#endif

void InitializeVulkan(const char* appName, SDL_Window* window);

void DrawFrame();

void CleanUpVulkan();

#endif
