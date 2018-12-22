// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2011, 2014 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <common.h>
#include <init.h>

static int dir320_console_init(void)
{
	barebox_set_hostname("dir320");

	return 0;
}
console_initcall(dir320_console_init);
