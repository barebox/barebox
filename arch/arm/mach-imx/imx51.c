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
#include <sizes.h>
#include <environment.h>
#include <io.h>
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

static int imx51_init(void)
{
	add_generic_device("imx_iim", 0, NULL, MX51_IIM_BASE_ADDR, SZ_4K,
			IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(imx51_init);

/*
 * Saves the boot source media into the $barebox_loc enviroment variable
 *
 * This information is useful for barebox init scripts as we can then easily
 * use a kernel image stored on the same media that we launch barebox with
 * (for example).
 *
 * imx25 and imx35 can boot into barebox from several media such as
 * nand, nor, mmc/sd cards, serial roms. "mmc" is used to represent several
 * sources as its impossible to distinguish between them.
 *
 * Some sources such as serial roms can themselves have 3 different boot
 * possibilities (i2c1, i2c2 etc). It is assumed that any board will
 * only be using one of these at any one time.
 *
 * Note also that I suspect that the boot source pins are only sampled at
 * power up.
 */

#define SRC_SBMR	0x4
#define SBMR_BT_MEM_TYPE_SHIFT	7
#define SBMR_BT_MEM_CTL_SHIFT	0
#define SBMR_BMOD_SHIFT		14

static int imx51_boot_save_loc(void)
{
	const char *bareboxloc = NULL;
	uint32_t reg;
	unsigned int ctrl, type;

	/* [CTRL][TYPE] */
	const char *const locations[4][4] = {
		{ /* CTRL = WEIM */
			"nor",
			NULL,
			"onenand",
			NULL,
		}, { /* CTRL == NAND */
			"nand",
			"nand",
			"nand",
			"nand",
		}, { /* CTRL == reserved */
			NULL,
			NULL,
			NULL,
			NULL,
		}, { /* CTRL == expansion */
			"mmc",
			NULL,
			"i2c",
			"spi",
		}
	};

	reg = readl(MX51_SRC_BASE_ADDR + SRC_SBMR);

	switch ((reg >> SBMR_BMOD_SHIFT) & 0x3) {
	case 0:
	case 2:
		/* internal boot */
		ctrl = (reg >> SBMR_BT_MEM_CTL_SHIFT) & 0x3;
		type = (reg >> SBMR_BT_MEM_TYPE_SHIFT) & 0x3;

		bareboxloc = locations[ctrl][type];
		break;
	case 1:
		/* reserved */
		bareboxloc = "unknown";
		break;
	case 3:
		bareboxloc = "serial";
		break;

	}

	if (bareboxloc) {
		setenv("barebox_loc", bareboxloc);
		export("barebox_loc");
	}

	return 0;
}

coredevice_initcall(imx51_boot_save_loc);
