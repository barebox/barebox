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

#endif
