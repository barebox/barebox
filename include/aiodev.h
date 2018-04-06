/*
 * core.c - Code implementing core functionality of AIODEV susbsystem
 *
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2015 Zodiac Inflight Innovation
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __AIODEVICE_H
#define __AIODEVICE_H

struct aiodevice;
struct aiochannel {
	int index;
	const char *unit;
	struct aiodevice *aiodev;

	int value;
	char *name;
};

struct aiodevice {
	const char *name;
	int (*read)(struct aiochannel *, int *val);
	struct device_d dev;
	struct device_d *hwdev;
	struct aiochannel **channels;
	int num_channels;
	struct list_head list;
};

int aiodevice_register(struct aiodevice *aiodev);

struct aiochannel *aiochannel_get(struct device_d *dev, int index);
struct aiochannel *aiochannel_get_by_name(const char *name);

int aiochannel_get_value(struct aiochannel *aiochan, int *value);
int aiochannel_get_index(struct aiochannel *aiochan);

static inline const char *aiochannel_get_unit(struct aiochannel *aiochan)
{
	return aiochan->unit;
}

extern struct list_head aiodevices;
#define for_each_aiodevice(aiodevice) list_for_each_entry(aiodevice, &aiodevices, list)

#endif
