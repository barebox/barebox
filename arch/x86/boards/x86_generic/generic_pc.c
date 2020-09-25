// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix

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
