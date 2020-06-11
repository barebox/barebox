// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Antony Pavlov <antonynpavlov@gmail.com>

/* This file is part of barebox. */

#include <common.h>
#include <init.h>

static int hostname_init(void)
{
	barebox_set_hostname("virt2real");

	return 0;
}
core_initcall(hostname_init);
