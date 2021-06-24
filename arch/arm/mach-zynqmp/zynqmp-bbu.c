// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Michael Tretter <m.tretter@pengutronix.de>
 */

#include <common.h>
#include <libfile.h>
#include <mach/zynqmp-bbu.h>

static int zynqmp_bbu_handler(struct bbu_handler *handler,
		struct bbu_data *data)
{
	int ret = 0;

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	ret = copy_file(data->imagefile, data->devicefile, 1);
	if (ret < 0) {
		pr_err("update failed: %s", strerror(-ret));
		return ret;
	}

	return ret;
}

int zynqmp_bbu_register_handler(const char *name, char *devicefile,
		unsigned long flags)
{
	struct bbu_handler *handler;
	int ret = 0;

	if (!name || !devicefile)
		return -EINVAL;

	handler = xzalloc(sizeof(*handler));
	handler->name = name;
	handler->devicefile = devicefile;
	handler->flags = flags;
	handler->handler = zynqmp_bbu_handler;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}
