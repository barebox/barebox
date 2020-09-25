/*
 * core.c - Code implementing core functionality of AIODEV susbsystem
 *
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2015 Zodiac Inflight Innovation
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
/* Find aiochannel by channel name, e.g. "aiodev0.in_value0_mV" */
struct aiochannel *aiochannel_by_name(const char *name);

int aiochannel_get_value(struct aiochannel *aiochan, int *value);
int aiochannel_get_index(struct aiochannel *aiochan);

static inline const char *aiochannel_get_unit(struct aiochannel *aiochan)
{
	return aiochan->unit;
}

extern struct list_head aiodevices;
#define for_each_aiodevice(aiodevice) list_for_each_entry(aiodevice, &aiodevices, list)

#endif
