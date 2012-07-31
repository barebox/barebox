/*
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

#include <common.h>
#include <init.h>

static int imx1_init(void)
{
	add_generic_device("imx-gpio", 0, NULL, 0x0021c000, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx-gpio", 1, NULL, 0x0021c100, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx-gpio", 2, NULL, 0x0021c200, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx-gpio", 3, NULL, 0x0021c300, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx1_init);
