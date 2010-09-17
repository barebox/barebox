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
#include <init.h>
#include <mach/imx-regs.h>
#include <mach/iim.h>
#include <asm/io.h>

#include "gpio.h"

void *imx_gpio_base[] = {
	(void *)0x53fcc000,
	(void *)0x53fd0000,
	(void *)0x53fa4000,
	(void *)0x53f9c000,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

u64 imx_uid(void)
{
	u64 uid = 0;
	int i;

	/*
	 * This code assumes that the UID is stored little-endian. The
	 * Freescale AN3682 document is silent about the endianess, but
	 * experimentation shows that this is the case. All other multi-byte
	 * values in IIM are big-endian as per AN3682.
	 */
	for (i = 0; i < 8; i++)
		uid |= (u64)readb(IIM_UID + i*4) << (i*8);

	return uid;
}

static struct imx_iim_platform_data imx25_iim_pdata = {
	.mac_addr_base	= IIM_MAC_ADDR,
};

static struct device_d imx25_iim_dev = {
	.id		= -1,
	.name		= "imx_iim",
	.map_base	= IMX_IIM_BASE,
	.platform_data	= &imx25_iim_pdata,
};

static struct device_d imx25_iim_bank0_dev = {
	.name		= "imx_iim_bank",
	.id		= 0,
	.map_base	= IIM_BANK0_BASE,
	.size		= IIM_BANK_SIZE,
};

static struct device_d imx25_iim_bank1_dev = {
	.name		= "imx_iim_bank",
	.id		= 1,
	.map_base	= IIM_BANK1_BASE,
	.size		= IIM_BANK_SIZE,
};

static struct device_d imx25_iim_bank2_dev = {
	.name		= "imx_iim_bank",
	.id		= 2,
	.map_base	= IIM_BANK2_BASE,
	.size		= IIM_BANK_SIZE,
};

static int imx25_iim_init(void)
{
	register_device(&imx25_iim_dev);
	register_device(&imx25_iim_bank0_dev);
	register_device(&imx25_iim_bank1_dev);
	register_device(&imx25_iim_bank2_dev);

	return 0;
}
coredevice_initcall(imx25_iim_init);
