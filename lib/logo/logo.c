/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <init.h>
#include <fs.h>
#include <libfile.h>
#include <malloc.h>

#define DEFINE_LOGO(width)							\
	extern char __bblogo_barebox_logo_w##width##_start[];			\
	extern char __bblogo_barebox_logo_w##width##_end[];			\
										\
	static void load_logo_##width(void)					\
	{									\
		if (!IS_ENABLED(CONFIG_BAREBOX_LOGO_##width))			\
			return;							\
		load_logo(width, __bblogo_barebox_logo_w##width##_start,	\
				__bblogo_barebox_logo_w##width##_end);		\
	}

static void load_logo(int width, void *start, void *end)
{
	char *filename;
	size_t size = end - start;

	filename = asprintf("/logo/barebox-logo-%d.png", width);
	write_file(filename, start, size);
	free(filename);
}

DEFINE_LOGO(64);
DEFINE_LOGO(240);
DEFINE_LOGO(320);
DEFINE_LOGO(400);
DEFINE_LOGO(640);

static int logo_init(void)
{
	int ret;

	ret = make_directory("/logo");
	if (ret)
		return ret;

	load_logo_64();
	load_logo_240();
	load_logo_320();
	load_logo_400();
	load_logo_640();

	return 0;
}
postenvironment_initcall(logo_init);
