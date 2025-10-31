#include <stdlib.h>

#include <SDL3/SDL_audio.h>

#include "music.h"

static SDL_AudioStream *stream = NULL;
static Uint8 *wav_data = NULL;
static Uint32 wav_data_len = 0;

void LoadMusic(const char* musicPath)
{
	Uint32 length;
	Uint8 * buffer;
	SDL_AudioSpec spec;

	if (SDL_LoadWAV(musicPath, &spec, &wav_data, &wav_data_len) == false)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to Load WAV at %s\n", musicPath);
		exit(EXIT_FAILURE);
	}

	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if(stream == false)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to open music : %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
}

void BeginPlayMusic()
{
	SDL_ResumeAudioStreamDevice(stream);
}

void FrameUpdateMusic()
{
	if (SDL_GetAudioStreamQueued(stream) < (int)wav_data_len) {
        /* feed more data to the stream. It will queue at the end, and trickle out as the hardware needs more data. */
        SDL_PutAudioStreamData(stream, wav_data, wav_data_len);
    }
}

void CloseMusic()
{
	SDL_DestroyAudioStream(stream);
	SDL_free(wav_data);
}
