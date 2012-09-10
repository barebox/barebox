/*
 * Copyright 2012 GE Intelligent Platforms, Inc
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <driver.h>
#include <mach/immap_85xx.h>
#include <mach/gianfar.h>

int fsl_eth_init(int num, struct gfar_info_struct *gf)
{
	struct resource *res;

	res = xzalloc(3 * sizeof(struct resource));
	/* TSEC interface registers */
	res[0].start = GFAR_BASE_ADDR + ((num - 1) * 0x1000);
	res[0].end = res[0].start + 0x1000 - 1;
	res[0].flags = IORESOURCE_MEM;
	/* External PHY access always through eTSEC1 */
	res[1].start = MDIO_BASE_ADDR;
	res[1].end = res[1].start + 0x1000 - 1;
	res[1].flags = IORESOURCE_MEM;
	/* Access to TBI/RTBI interface. */
	res[2].start = MDIO_BASE_ADDR + ((num - 1) * 0x1000);
	res[2].end = res[2].start + 0x1000 - 1;
	res[2].flags = IORESOURCE_MEM;

	add_generic_device_res("gfar", DEVICE_ID_DYNAMIC, res, 3, gf);

	return 0;
}
