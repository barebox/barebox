/*
 * Copyright (C) 2010 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <module.h>
#include <param.h>
#include <poller.h>

static LIST_HEAD(poller_list);
static int poller_active;

int poller_register(struct poller_struct *poller)
{
	list_add_tail(&poller->list, &poller_list);

	return 0;
}

int poller_unregister(struct poller_struct *poller)
{
	list_del(&poller->list);

	return 0;
}

void poller_call(void)
{
	struct poller_struct *poller, *tmp;

	if (poller_active)
		return;

	poller_active = 1;

	list_for_each_entry_safe(poller, tmp, &poller_list, list)
		poller->func(poller);

	poller_active = 0;
}
