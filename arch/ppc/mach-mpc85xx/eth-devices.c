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
#include <init.h>
#include <mach/immap_85xx.h>
#include <mach/gianfar.h>

static int fsl_phy_init(void)
{
	int i;
	void __iomem *base  = IOMEM(GFAR_BASE_ADDR + GFAR_TBIPA_OFFSET);

	/*
	 * The TBI address must be initialised to enable the PHY to
	 * link up after the MDIO reset.
	 */
	out_be32(base, GFAR_TBIPA_END);
	/* All ports access external PHYs via the "gfar-mdio" device */
	add_generic_device("gfar-mdio", 0, NULL, MDIO_BASE_ADDR,
			0x1000, IORESOURCE_MEM, NULL);

	for (i = 1; i < 3; i++) {
		out_be32(base + (i * 0x1000), GFAR_TBIPA_END - i);
		/* Use "gfar-tbiphy" devices to access internal PHY. */
		add_generic_device("gfar-tbiphy", i, NULL,
				MDIO_BASE_ADDR + (i * 0x1000),
				0x1000, IORESOURCE_MEM, NULL);
	}
	return 0;
}

coredevice_initcall(fsl_phy_init);

int fsl_eth_init(int num, struct gfar_info_struct *gf)
{
	add_generic_device("gfar", DEVICE_ID_DYNAMIC, NULL,
			GFAR_BASE_ADDR + ((num - 1) * 0x1000), 0x1000,
			IORESOURCE_MEM, gf);
	return 0;
}
