/*
 * board.c
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <envfs.h>
#include <driver.h>
#include <malloc.h>
#include <mach/linux.h>
#include <init.h>
#include <errno.h>
#include <deep-probe.h>
#include <fb.h>

struct fb_videomode mode = {
	.name = "sdl",	/* optional */
	.xres = 640,
	.yres = 480,
};

static struct device tap_device = {
	.id	  = DEVICE_ID_DYNAMIC,
	.name     = "tap",
};

static struct device sdl_device = {
	.id	  = DEVICE_ID_DYNAMIC,
	.name     = "sdlfb",
	.platform_data = &mode,
};

static struct device devrandom_device = {
	.id	  = DEVICE_ID_DYNAMIC,
	.name     = "devrandom",
};

static int devices_init(struct device *dev)
{
	platform_device_register(&tap_device);

	if (sdl_xres)
		mode.xres = sdl_xres;

	if (sdl_yres)
		mode.yres = sdl_yres;

	platform_device_register(&sdl_device);

	platform_device_register(&devrandom_device);

	defaultenv_append_directory(defaultenv_sandbox);

	return 0;
}

static struct of_device_id sandbox_dt_ids[] = {
	{ .compatible = "barebox,sandbox" },
	{ /* sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(sandbox_dt_ids);

static struct driver sandbox_board_drv = {
	.name  = "sandbox-board",
	.of_compatible = sandbox_dt_ids,
	.probe = devices_init,
};
device_platform_driver(sandbox_board_drv);
