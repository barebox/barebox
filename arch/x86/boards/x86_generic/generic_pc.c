/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 *
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <asm/syslib.h>
#include <linux/err.h>

static int devices_init(void)
{
	/* extended memory only */
	add_mem_device("ram0", 0x0, bios_get_memsize() << 10,
		       IORESOURCE_MEM_WRITEABLE);
	return 0;
}
device_initcall(devices_init);
