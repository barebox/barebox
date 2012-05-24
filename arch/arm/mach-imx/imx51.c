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
#include <mach/imx5.h>
#include <mach/imx-regs.h>
#include <mach/clock-imx51_53.h>

#include "gpio.h"

void *imx_gpio_base[] = {
	(void *)MX51_GPIO1_BASE_ADDR,
	(void *)MX51_GPIO2_BASE_ADDR,
	(void *)MX51_GPIO3_BASE_ADDR,
	(void *)MX51_GPIO4_BASE_ADDR,
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
		mx51_silicon_revision = IMX_CHIP_REV_1_0;
		mx51_rev_string = "1.0";
		break;
	case 0x2:
		mx51_silicon_revision = IMX_CHIP_REV_1_1;
		mx51_rev_string = "1.1";
		break;
	case 0x10:
		mx51_silicon_revision = IMX_CHIP_REV_2_0;
		mx51_rev_string = "2.0";
		break;
	case 0x20:
		mx51_silicon_revision = IMX_CHIP_REV_3_0;
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

#define setup_pll_800(base)	imx5_setup_pll((base), 800,  (( 8 << 4) + ((1 - 1) << 0)), ( 3 - 1),  1)
#define setup_pll_665(base)	imx5_setup_pll((base), 665,  (( 6 << 4) + ((1 - 1) << 0)), (96 - 1), 89)
#define setup_pll_600(base)	imx5_setup_pll((base), 600,  (( 6 << 4) + ((1 - 1) << 0)), ( 4 - 1),  1)
#define setup_pll_400(base)	imx5_setup_pll((base), 400,  (( 8 << 4) + ((2 - 1) << 0)), ( 3 - 1),  1)
#define setup_pll_455(base)	imx5_setup_pll((base), 455,  (( 9 << 4) + ((2 - 1) << 0)), (48 - 1), 23)
#define setup_pll_216(base)	imx5_setup_pll((base), 216,  (( 6 << 4) + ((3 - 1) << 0)), ( 4 - 1),  3)

void imx51_init_lowlevel(unsigned int cpufreq_mhz)
{
	void __iomem *ccm = (void __iomem *)MX51_CCM_BASE_ADDR;
	u32 r;

	imx5_init_lowlevel();

	/* disable write combine for TO 2 and lower revs */
	if (imx_silicon_revision() < IMX_CHIP_REV_3_0) {
		__asm__ __volatile__("mrc 15, 1, %0, c9, c0, 1":"=r"(r));
		r |= (1 << 25);
		__asm__ __volatile__("mcr 15, 1, %0, c9, c0, 1" : : "r"(r));
	}

	/* Gate of clocks to the peripherals first */
	writel(0x3fffffff, ccm + MX5_CCM_CCGR0);
	writel(0x00000000, ccm + MX5_CCM_CCGR1);
	writel(0x00000000, ccm + MX5_CCM_CCGR2);
	writel(0x00000000, ccm + MX5_CCM_CCGR3);
	writel(0x00030000, ccm + MX5_CCM_CCGR4);
	writel(0x00fff030, ccm + MX5_CCM_CCGR5);
	writel(0x00000300, ccm + MX5_CCM_CCGR6);

	/* Disable IPU and HSC dividers */
	writel(0x00060000, ccm + MX5_CCM_CCDR);

	/* Make sure to switch the DDR away from PLL 1 */
	writel(0x19239145, ccm + MX5_CCM_CBCDR);
	/* make sure divider effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	/* Switch ARM to step clock */
	writel(0x4, ccm + MX5_CCM_CCSR);

	switch (cpufreq_mhz) {
	case 600:
		setup_pll_600((void __iomem *)MX51_PLL1_BASE_ADDR);
		break;
	default:
		/* Default maximum 800MHz */
		setup_pll_800((void __iomem *)MX51_PLL1_BASE_ADDR);
		break;
	}

	setup_pll_665((void __iomem *)MX51_PLL3_BASE_ADDR);

	/* Switch peripheral to PLL 3 */
	writel(0x000010C0, ccm + MX5_CCM_CBCMR);
	writel(0x13239145, ccm + MX5_CCM_CBCDR);

	setup_pll_665((void __iomem *)MX51_PLL2_BASE_ADDR);

	/* Switch peripheral to PLL2 */
	writel(0x19239145, ccm + MX5_CCM_CBCDR);
	writel(0x000020C0, ccm + MX5_CCM_CBCMR);

	setup_pll_216((void __iomem *)MX51_PLL3_BASE_ADDR);

	/* Set the platform clock dividers */
	writel(0x00000124, MX51_ARM_BASE_ADDR + 0x14);

	/* Run at Full speed */
	writel(0x0, ccm + MX5_CCM_CACRR);

	/* Switch ARM back to PLL 1 */
	writel(0x0, ccm + MX5_CCM_CCSR);

	/* setup the rest */
	/* Use lp_apm (24MHz) source for perclk */
	writel(0x000020C2, ccm + MX5_CCM_CBCMR);
	/* ddr clock from PLL 1, all perclk dividers are 1 since using 24MHz */
	writel(0x59239100, ccm + MX5_CCM_CBCDR);

	/* Restore the default values in the Gate registers */
	writel(0xffffffff, ccm + MX5_CCM_CCGR0);
	writel(0xffffffff, ccm + MX5_CCM_CCGR1);
	writel(0xffffffff, ccm + MX5_CCM_CCGR2);
	writel(0xffffffff, ccm + MX5_CCM_CCGR3);
	writel(0xffffffff, ccm + MX5_CCM_CCGR4);
	writel(0xffffffff, ccm + MX5_CCM_CCGR5);
	writel(0xffffffff, ccm + MX5_CCM_CCGR6);

	/* Use PLL 2 for UART's, get 66.5MHz from it */
	writel(0xA591A020, ccm + MX5_CCM_CSCMR1);
	writel(0x00C30321, ccm + MX5_CCM_CSCDR1);

	/* make sure divider effective */
	while (readl(ccm + MX5_CCM_CDHIPR));

	writel(0x0, ccm + MX5_CCM_CCDR);
}
