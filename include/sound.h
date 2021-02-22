/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: © 2021 Ahmad Fatoum */

#ifndef __SOUND_H_
#define __SOUND_H_

#include <linux/list.h>
#include <poller.h>

#define BELL_DEFAULT_FREQUENCY	-1

struct sound_card {
	const char *name;
	int bell_frequency;
	int (*beep)(struct sound_card *, unsigned freq, unsigned us);

	/* private */
	struct list_head list;
	struct list_head tune;
	struct poller_async poller;
};

int sound_card_register(struct sound_card *card);
int sound_card_beep_wait(struct sound_card *card, unsigned timeout_us);
int sound_card_beep(struct sound_card *card, int freq, unsigned int us);
int sound_card_beep_cancel(struct sound_card *card);

struct sound_card *sound_card_get_default(void);

static inline int beep(int freq, unsigned us)
{
	return sound_card_beep(sound_card_get_default(), freq, us);
}

static inline int beep_wait(unsigned timeout_us)
{
	return sound_card_beep_wait(sound_card_get_default(), timeout_us);
}

static inline int beep_cancel(void)
{
	return sound_card_beep_cancel(sound_card_get_default());
}


/*
 * Synthesizers all have the same prototype, but their implementation
 * is replaced with a gain-adjusted square wave if CONFIG_SYNTH_SQUARES=y.
 * This is to support PCM beeping on systems, where sine generation may
 * spend to much time in the poller.
 */
typedef void synth_t(unsigned freq, s16 amplitude, s16 *stream,
		     unsigned sample_rate, unsigned nsamples);

#ifdef CONFIG_SYNTH_SQUARES
#define SYNTH(fn, volume_percent)                                \
	static inline void fn(unsigned f, s16 amplitude, s16 *s, \
			      unsigned r, unsigned n) {          \
		synth_t __synth_square;                          \
		amplitude = amplitude * volume_percent / 100;    \
		__synth_square(f, amplitude, s, r, n);           \
	}                                                        \
	synth_t __##fn
#else
#define SYNTH(fn, volume_percent)                                \
	static inline void fn(unsigned f, s16 a, s16 *s,         \
			      unsigned r, unsigned n) {          \
		synth_t __##fn;                                  \
		__##fn(f, a, s, r, n);                           \
	}                                                        \
        synth_t __##fn
#endif

SYNTH(synth_square, 100);	/* square wave always has full amplitude */
SYNTH(synth_sin, 64);		/* ∫₀¹ sin(πx) dx ≈ 64% */

#endif
