/*
 * board.c
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <mach/linux.h>
#include <init.h>
#include <errno.h>
#include <fb.h>

struct fb_videomode mode = {
	.name = "sdl",	/* optional */
	.xres = 640,
	.yres = 480,
};

static struct device_d tap_device = {
	.id	  = DEVICE_ID_DYNAMIC,
	.name     = "tap",
};

static struct device_d sdl_device = {
	.id	  = DEVICE_ID_DYNAMIC,
	.name     = "sdlfb",
	.platform_data = &mode,
};

static int devices_init(void)
{
	register_device(&tap_device);

	if (sdl_xres)
		mode.xres = sdl_xres;

	if (sdl_yres)
		mode.yres = sdl_yres;

	register_device(&sdl_device);

	return 0;
}

device_initcall(devices_init);

