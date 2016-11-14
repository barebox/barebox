/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#include <stdio.h>
#include <SDL.h>
#include <time.h>
#include <signal.h>
#include <mach/linux.h>
#include <unistd.h>
#include <pthread.h>

struct fb_bitfield {
	uint32_t offset;			/* beginning of bitfield	*/
	uint32_t length;			/* length of bitfield		*/
	uint32_t msb_right;			/* != 0 : Most significant bit is */
					/* right */
};

static SDL_Surface *real_screen;
static void *buffer = NULL;
pthread_t th;

static void sdl_copy_buffer(SDL_Surface *screen)
{
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return;
	}

	memcpy(screen->pixels, buffer, screen->pitch * screen->h);

	if(SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
}

static void *threadStart(void *ptr)
{
	while (1) {
		usleep(1000 * 100);

		sdl_copy_buffer(real_screen);
		SDL_Flip(real_screen);
	}

	return 0;
}

void sdl_start_timer(void)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&th, &attr, threadStart, NULL);
}

void sdl_stop_timer(void)
{
	pthread_cancel(th);
}

void sdl_get_bitfield_rgba(struct fb_bitfield *r, struct fb_bitfield *g,
			    struct fb_bitfield *b, struct fb_bitfield *a)
{
	SDL_Surface *screen = real_screen;

	r->length = 8 - screen->format->Rloss;
	r->offset = screen->format->Rshift;
	g->length = 8 - screen->format->Gloss;
	g->offset = screen->format->Gshift;
	b->length = 8 - screen->format->Bloss;
	b->offset = screen->format->Bshift;
	a->length = 8 - screen->format->Aloss;
	a->offset = screen->format->Ashift;
}

int sdl_open(int xres, int yres, int bpp, void* buf)
{
	int flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return -1;
	}

	real_screen = SDL_SetVideoMode(xres, yres, bpp, flags);
	if (!real_screen) {
		sdl_close();
		fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
		return -1;
	}

	buffer = buf;

	return 0;
}

void sdl_close(void)
{
	sdl_stop_timer();
	SDL_Quit();
}
