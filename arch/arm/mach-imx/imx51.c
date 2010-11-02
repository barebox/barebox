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

#include <init.h>
#include <common.h>
#include <asm/io.h>
#include <mach/imx51-regs.h>

#include "gpio.h"

void *imx_gpio_base[] = {
	(void *)0x87f84000,
	(void *)0x73f88000,
	(void *)0x73f8c000,
	(void *)0x73f90000,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

#define SI_REV 0x48

static u32 mx51_silicon_revision;
static char *mx51_rev_string = "unknown";

int imx_silicon_revision(void)
{
	return mx51_silicon_revision;
}

static int query_silicon_revision(void)
{
	void __iomem *rom = MX51_IROM_BASE_ADDR;
	u32 rev;

	rev = readl(rom + SI_REV);
	switch (rev) {
	case 0x1:
		mx51_silicon_revision = MX51_CHIP_REV_1_0;
		mx51_rev_string = "1.0";
		break;
	case 0x2:
		mx51_silicon_revision = MX51_CHIP_REV_1_1;
		mx51_rev_string = "1.1";
		break;
	case 0x10:
		mx51_silicon_revision = MX51_CHIP_REV_2_0;
		mx51_rev_string = "2.0";
		break;
	case 0x20:
		mx51_silicon_revision = MX51_CHIP_REV_3_0;
		mx51_rev_string = "3.0";
		break;
	default:
		mx51_silicon_revision = 0;
	}

	return 0;
}
core_initcall(query_silicon_revision);

static int imx51_print_silicon_rev(void)
{
	printf("detected i.MX51 rev %s\n", mx51_rev_string);

	return 0;
}
device_initcall(imx51_print_silicon_rev);
