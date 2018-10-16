/*
 * core.c - Code implementing core functionality of AIODEV susbsystem
 *
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * Copyright (c) 2015 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
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

#include <common.h>
#include <aiodev.h>
#include <linux/list.h>
#include <malloc.h>

LIST_HEAD(aiodevices);
EXPORT_SYMBOL(aiodevices);

struct aiochannel *aiochannel_get_by_name(const char *name)
{
	struct aiodevice *aiodev;
	int i;

	list_for_each_entry(aiodev, &aiodevices, list) {
		for (i = 0; i < aiodev->num_channels; i++)
			if (!strcmp(name, aiodev->channels[i]->name))
				return aiodev->channels[i];
	}

	return ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL(aiochannel_get_by_name);

struct aiochannel *aiochannel_get(struct device_d *dev, int index)
{
	struct of_phandle_args spec;
	struct aiodevice *aiodev;
	int ret, chnum = 0;

	if (!dev->device_node)
		return ERR_PTR(-EINVAL);

	ret = of_parse_phandle_with_args(dev->device_node,
					 "io-channels",
					 "#io-channel-cells",
					 index, &spec);
        if (ret)
                return ERR_PTR(ret);

	list_for_each_entry(aiodev, &aiodevices, list) {
		if (aiodev->hwdev->device_node == spec.np)
			goto found;
	}

	return ERR_PTR(-EPROBE_DEFER);

found:
	if (spec.args_count)
		chnum = spec.args[0];

	if (chnum >= aiodev->num_channels)
		return ERR_PTR(-EINVAL);

	return aiodev->channels[chnum];
}
EXPORT_SYMBOL(aiochannel_get);

int aiochannel_get_value(struct aiochannel *aiochan, int *value)
{
	struct aiodevice *aiodev = aiochan->aiodev;

	return aiodev->read(aiochan, value);
}
EXPORT_SYMBOL(aiochannel_get_value);

int aiochannel_get_index(struct aiochannel *aiochan)
{
	return aiochan->index;
}
EXPORT_SYMBOL(aiochannel_get_index);

static int aiochannel_param_get_value(struct param_d *p, void *priv)
{
	struct aiochannel *aiochan = priv;

	return aiochannel_get_value(aiochan, &aiochan->value);
}

int aiodevice_register(struct aiodevice *aiodev)
{
	int i, ret;

	if (!aiodev->name && aiodev->hwdev &&
	    aiodev->hwdev->device_node) {
		aiodev->dev.id = DEVICE_ID_SINGLE;

		aiodev->name = of_alias_get(aiodev->hwdev->device_node);
	}

	if (!aiodev->name) {
		aiodev->name = "aiodev";
		aiodev->dev.id = DEVICE_ID_DYNAMIC;
	}

	dev_set_name(&aiodev->dev, aiodev->name);

	aiodev->dev.parent = aiodev->hwdev;

	ret = register_device(&aiodev->dev);
	if (ret)
		return ret;

	for (i = 0; i < aiodev->num_channels; i++) {
		struct aiochannel *aiochan = aiodev->channels[i];
		char *name;

		aiochan->index  = i;
		aiochan->aiodev = aiodev;

		name = xasprintf("in_value%d_%s", i, aiochan->unit);

		dev_add_param_int(&aiodev->dev, name, NULL,
				  aiochannel_param_get_value,
				  &aiochan->value, "%d", aiochan);

		aiochan->name = xasprintf("%s.%s", aiodev->name, name);

		free(name);
	}

	list_add_tail(&aiodev->list, &aiodevices);

	return 0;
}
EXPORT_SYMBOL(aiodevice_register);
