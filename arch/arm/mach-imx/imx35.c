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
#include <sizes.h>
#include <init.h>
#include <io.h>
#include <mach/imx-regs.h>
#include <mach/iim.h>
#include <mach/generic.h>

int imx_silicon_revision()
{
	uint32_t reg;
	reg = readl(IMX_IIM_BASE + IIM_SREV);
	/* 0×00 = TO 1.0, First silicon */
	reg += IMX_CHIP_REV_1_0;

	return (reg & 0xFF);
}

/*
 * There are some i.MX35 CPUs in the wild, comming with bogus L2 cache settings.
 * These misconfigured CPUs will run amok immediately when the L2 cache gets
 * enabled. Workaraound is to setup the correct register setting prior enabling
 * the L2 cache. This should not hurt already working CPUs, as they are using the
 * same value
 */

#define L2_MEM_VAL 0x10

static int imx35_l2_fix(void)
{
	writel(0x515, IMX_CLKCTL_BASE + L2_MEM_VAL);

	return 0;
}
core_initcall(imx35_l2_fix);

static int imx35_init(void)
{
	add_generic_device("imx_iim", 0, NULL, IMX_IIM_BASE, SZ_4K,
			IORESOURCE_MEM, NULL);

	add_generic_device("imx31-gpio", 0, NULL, 0x53fcc000, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, 0x53fd0000, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, 0x53fa4000, 0x1000, IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx35_init);
