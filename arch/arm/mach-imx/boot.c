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
#include <environment.h>
#include <init.h>

#include <asm/io.h>
#include <mach/imx-regs.h>

#if defined(CONFIG_ARCH_IMX25) || defined(CONFIG_ARCH_IMX35)
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
static int imx_boot_save_loc(void)
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
		}, { /* CTRL == ATA, (imx35 only) */
			NULL,
			NULL, /* might be p-ata */
			NULL,
			NULL,
		}, { /* CTRL == expansion */
			"mmc", /* note imx25 could also be: movinand, ce-ata */
			NULL,
			"i2c",
			"spi",
		}
	};

	reg = readl(IMX_CCM_BASE + CCM_RCSR);
	ctrl = (reg >> CCM_RCSR_MEM_CTRL_SHIFT) & 0x3;
	type = (reg >> CCM_RCSR_MEM_TYPE_SHIFT) & 0x3;

	bareboxloc = locations[ctrl][type];

	if (bareboxloc) {
		setenv("barebox_loc", bareboxloc);
		export("barebox_loc");
	}

	return 0;
}

/*
 * This can only be called after env_push_context() has been called
 * so it is a late_initcall.
 */
late_initcall(imx_boot_save_loc);

#endif
