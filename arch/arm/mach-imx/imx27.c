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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <mach/imx-regs.h>
#include <sizes.h>
#include <init.h>

#include "gpio.h"

int imx_silicon_revision(void)
{
	return CID >> 28;
}

void *imx_gpio_base[] = {
	(void *)0x10015000,
	(void *)0x10015100,
	(void *)0x10015200,
	(void *)0x10015300,
	(void *)0x10015400,
	(void *)0x10015500,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

static int imx27_init(void)
{
	add_generic_device("imx_iim", 0, NULL, IMX_IIM_BASE, SZ_4K,
			IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx27_init);
