// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021 Ahmad Fatoum
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <SDL.h>
#include <mach/linux.h>

static void sdl_perror(const char *what)
{
	printf("SDL: Could not %s: %s.\n", what, SDL_GetError());
}

static struct sdl_fb_info info;
static SDL_atomic_t shutdown;
static SDL_Window *window;

static void handle_sdl_events(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT)
			SDL_AtomicSet(&shutdown, true);
	}
}

static int scanout(void *ptr)
{
	SDL_Renderer *renderer;
	SDL_Surface *surface;
	SDL_Texture *texture;
	void *buf = info.screen_base;
	int ret = -1;

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		sdl_perror("create renderer");
		return -1;
	}

	surface = SDL_CreateRGBSurface(0, info.xres, info.yres, info.bpp,
				       info.rmask, info.gmask, info.bmask, info.amask);
	if (!surface) {
		sdl_perror("create surface");
		goto destroy_renderer;
	}

	texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		sdl_perror("create texture");
		goto free_surface;
	}

	while (!SDL_AtomicGet(&shutdown)) {
		SDL_Delay(100);

		handle_sdl_events();  /* Handle events like window close */
		SDL_UpdateTexture(texture, NULL, buf, surface->pitch);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	ret = 0;

	SDL_DestroyTexture(texture);
free_surface:
	SDL_FreeSurface(surface);
destroy_renderer:
	SDL_DestroyRenderer(renderer);

	return ret;
}

static SDL_Thread *thread;

void sdl_video_close(void)
{
	SDL_AtomicSet(&shutdown, true); /* implies full memory barrier */
	SDL_WaitThread(thread, NULL);
	SDL_AtomicSet(&shutdown, false);
	SDL_DestroyWindow(window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

int sdl_video_open(const struct sdl_fb_info *_info)
{
	info = *_info;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		sdl_perror("initialize SDL Video");
		return -1;
	}

	window = SDL_CreateWindow("barebox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				  info.xres, info.yres, 0);
	if (!window) {
		sdl_perror("create window");
		goto quit_subsystem;
	}

	/* All scanout needs to happen in the same thread, because not all
	 * graphic backends are thread-safe. The window is created in the main
	 * thread though to work around libEGL crashing with SDL_VIDEODRIVER=wayland
	 */

	thread = SDL_CreateThread(scanout, "video-scanout", NULL);
	if (!thread) {
		sdl_perror("start scanout thread");
		goto destroy_window;
	}

	return 0;

destroy_window:
	SDL_DestroyWindow(window);
quit_subsystem:
	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	return -1;
}

static SDL_AudioDeviceID dev;

int sdl_sound_init(unsigned sample_rate)
{
	SDL_AudioSpec audiospec = {
		.freq = sample_rate,
		.format = AUDIO_S16,
		.channels = 1,
		.samples = 2048,
	};

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		sdl_perror("initialize SDL Audio");
		return -1;
	}

	dev = SDL_OpenAudioDevice(NULL, 0, &audiospec, NULL, 0);
	if (!dev) {
		sdl_perror("initialize open audio device");
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return -1;
	}

	SDL_PauseAudioDevice(dev, 0);
	return 0;
}

void sdl_sound_close(void)
{
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int sdl_sound_play(const void *data, unsigned nsamples)
{
	/* core sound support handles all the queueing for us */
	SDL_ClearQueuedAudio(dev);
	return SDL_QueueAudio(dev, data, nsamples * sizeof(uint16_t));
}

void sdl_sound_stop(void)
{
	SDL_ClearQueuedAudio(dev);
}
