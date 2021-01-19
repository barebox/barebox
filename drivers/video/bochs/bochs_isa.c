// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Copyright (c) 2020 Ahmad Fatoum, Pengutronix
/*
 *  ISA driver entry point for VGA with the Bochs VBE / QEMU stdvga extensions.
 */

#include <common.h>
#include <driver.h>
#include <linux/ioport.h>
#include "bochs_hw.h"

static int bochs_isa_detect(void)
{
	struct device_d *dev;
	int ret;

	outw(0, VBE_DISPI_IOPORT_INDEX);
	ret = inw(VBE_DISPI_IOPORT_DATA);

	if ((ret & 0xB0C0) != 0xB0C0)
		return -ENODEV;

	dev = device_alloc("bochs-dispi", 0);

	ret = platform_device_register(dev);
	if (ret)
		return ret;

	return bochs_hw_probe(dev, (void *)0xe0000000, NULL);
}
device_initcall(bochs_isa_detect);
