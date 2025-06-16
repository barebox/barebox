/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
	static __maybe_unused void load_logo_##width(void)			\
	{									\
		load_logo(width, __bblogo_barebox_logo_w##width##_start,	\
				__bblogo_barebox_logo_w##width##_end);		\
	}

static void load_logo(int width, void *start, void *end)
{
	char *filename;
	size_t size = end - start;
	char *ext = "";

	if (IS_ENABLED(CONFIG_BAREBOX_LOGO_PNG))
		ext = "png";
	else if (IS_ENABLED(CONFIG_BAREBOX_LOGO_BMP))
		ext = "bmp";
	else if (IS_ENABLED(CONFIG_BAREBOX_LOGO_QOI))
		ext = "qoi";

	filename = basprintf("/logo/barebox-logo-%d.%s", width, ext);
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

	/*
	 * fixdep depends on CONFIG_ symbols being listed in the file
	 * as-is with no preprocessor magic involved
	 */
	IF_ENABLED(CONFIG_BAREBOX_LOGO_64,  load_logo_64());
	IF_ENABLED(CONFIG_BAREBOX_LOGO_240, load_logo_240());
	IF_ENABLED(CONFIG_BAREBOX_LOGO_320, load_logo_320());
	IF_ENABLED(CONFIG_BAREBOX_LOGO_400, load_logo_400());
	IF_ENABLED(CONFIG_BAREBOX_LOGO_640, load_logo_640());

	return 0;
}
postenvironment_initcall(logo_init);
