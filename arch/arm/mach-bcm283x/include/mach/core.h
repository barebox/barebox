/*
 * Copyright (C) 2009 Carlo Caione <carlo@carlocaione.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __BCM2835_CORE_H__
#define __BCM2835_CORE_H__

#include <mach/platform.h>

void bcm2835_add_device_sdram(u32 size);

static void inline bcm2835_register_fb(void)
{
	add_generic_device("bcm2835_fb", 0, NULL, 0, 0, 0, NULL);
}

#endif
