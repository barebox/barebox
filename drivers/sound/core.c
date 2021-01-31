// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2021 Ahmad Fatoum

#include <common.h>
#include <linux/list.h>
#include <sound.h>
#include <poller.h>
#include <linux/iopoll.h>

static LIST_HEAD(card_list);

struct beep {
	int freq;
	unsigned int us;
	struct list_head list;
};

static int sound_card_do_beep(struct sound_card *card,
			      int freq, unsigned int us)
{
	if (freq == -1)
		freq = card->bell_frequency;

	return card->beep(card, freq, us);
}

static void sound_card_poller_cb(void *_card)
{
	struct sound_card *card = _card;
	struct beep *beep;

	beep = list_first_entry_or_null(&card->tune, struct beep, list);
	if (!beep) {
		sound_card_do_beep(card, 0, 0);
		return;
	}

	list_del(&beep->list);

	poller_call_async(&card->poller, beep->us * 1000ULL,
			  sound_card_poller_cb, card);
	sound_card_do_beep(card, beep->freq, beep->us);

	free(beep);
}

int sound_card_register(struct sound_card *card)
{
	if (!card->name)
		return -EINVAL;

	if (card->bell_frequency <= 0)
		card->bell_frequency = 1000;

	poller_async_register(&card->poller, card->name);
	INIT_LIST_HEAD(&card->tune);

	list_add_tail(&card->list, &card_list);
	return 0;
}

struct sound_card *sound_card_get_default(void)
{
	return list_first_entry_or_null(&card_list, struct sound_card, list);
}

int sound_card_beep(struct sound_card *card, int freq, unsigned int us)
{
	struct beep *beep;
	int ret;

	if (!card)
		return -ENODEV;

	if (!poller_async_active(&card->poller)) {
		ret = sound_card_do_beep(card, freq, us);
		if (!ret)
			poller_call_async(&card->poller, us * 1000ULL,
					  sound_card_poller_cb, card);

		return ret;
	}

	beep = malloc(sizeof(*beep));
	if (!beep)
		return -ENOMEM;

	beep->freq = freq;
	beep->us = us;

	list_add_tail(&beep->list, &card->tune);

	return 0;
}

int sound_card_beep_wait(struct sound_card *card, unsigned timeout)
{
	bool active;
	return read_poll_timeout(poller_async_active, active,
				 !active, timeout, &card->poller);
}

int sound_card_beep_cancel(struct sound_card *card)
{
	struct beep *beep, *tmp;
	int ret;

	if (!card)
		return -ENODEV;

	poller_async_cancel(&card->poller);

	ret = card->beep(card, 0, 0);

	list_for_each_entry_safe(beep, tmp, &card->tune, list) {
		list_del(&beep->list);
		free(beep);
	}

	return ret;
}
