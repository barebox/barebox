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
#include <environment.h>
#include <init.h>
#include <magicvar.h>

#include <io.h>
#include <mach/generic.h>

static const char *bootsource_str[] = {
	[bootsource_unknown] = "unknown",
	[bootsource_nand] = "nand",
	[bootsource_nor] = "nor",
	[bootsource_mmc] = "mmc",
	[bootsource_i2c] = "i2c",
	[bootsource_spi] = "spi",
	[bootsource_serial] = "serial",
	[bootsource_onenand] = "onenand",
	[bootsource_hd] = "harddisk",
};

static enum imx_bootsource bootsource;

void imx_set_bootsource(enum imx_bootsource src)
{
	if (src >= ARRAY_SIZE(bootsource_str))
		src = bootsource_unknown;

	bootsource = src;

	setenv("barebox_loc", bootsource_str[src]);
	export("barebox_loc");
}

enum imx_bootsource imx_bootsource(void)
{
	return bootsource;
}

BAREBOX_MAGICVAR(barebox_loc, "The source barebox has been booted from");

/* [CTRL][TYPE] */
static const enum imx_bootsource locations[4][4] = {
	{ /* CTRL = WEIM */
		bootsource_nor,
		bootsource_unknown,
		bootsource_onenand,
		bootsource_unknown,
	}, { /* CTRL == NAND */
		bootsource_nand,
		bootsource_nand,
		bootsource_nand,
		bootsource_nand,
	}, { /* CTRL == ATA, (imx35 only) */
		bootsource_unknown,
		bootsource_unknown, /* might be p-ata */
		bootsource_unknown,
		bootsource_unknown,
	}, { /* CTRL == expansion */
		bootsource_mmc, /* note imx25 could also be: movinand, ce-ata */
		bootsource_unknown,
		bootsource_i2c,
		bootsource_spi,
	}
};

/*
 * Saves the boot source media into the $barebox_loc environment variable
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
int imx_25_35_boot_save_loc(unsigned int ctrl, unsigned int type)
{
	const char *bareboxloc = NULL;
	enum imx_bootsource src;

	src = locations[ctrl][type];

	imx_set_bootsource(src);
	if (bareboxloc) {
		setenv("barebox_loc", bareboxloc);
		export("barebox_loc");
	}

	return 0;
}

#define IMX27_SYSCTRL_GPCR	0x18
#define IMX27_GPCR_BOOT_SHIFT			16
#define IMX27_GPCR_BOOT_MASK			(0xf << IMX27_GPCR_BOOT_SHIFT)
#define IMX27_GPCR_BOOT_UART_USB		0
#define IMX27_GPCR_BOOT_8BIT_NAND_2k		2
#define IMX27_GPCR_BOOT_16BIT_NAND_2k		3
#define IMX27_GPCR_BOOT_16BIT_NAND_512		4
#define IMX27_GPCR_BOOT_16BIT_CS0		5
#define IMX27_GPCR_BOOT_32BIT_CS0		6
#define IMX27_GPCR_BOOT_8BIT_NAND_512		7

void imx_27_boot_save_loc(void __iomem *sysctrl_base)
{
	enum imx_bootsource src;
	uint32_t val;

	val = readl(sysctrl_base + IMX27_SYSCTRL_GPCR);
	val &= IMX27_GPCR_BOOT_MASK;
	val >>= IMX27_GPCR_BOOT_SHIFT;

	switch (val) {
	case IMX27_GPCR_BOOT_UART_USB:
		src = bootsource_serial;
		break;
	case IMX27_GPCR_BOOT_8BIT_NAND_2k:
	case IMX27_GPCR_BOOT_16BIT_NAND_2k:
	case IMX27_GPCR_BOOT_16BIT_NAND_512:
	case IMX27_GPCR_BOOT_8BIT_NAND_512:
		src = bootsource_nand;
		break;
	default:
		src = bootsource_nor;
		break;
	}

	imx_set_bootsource(src);
}

#define IMX51_SRC_SBMR			0x4
#define IMX51_SBMR_BT_MEM_TYPE_SHIFT	7
#define IMX51_SBMR_BT_MEM_CTL_SHIFT	0
#define IMX51_SBMR_BMOD_SHIFT		14

int imx51_boot_save_loc(void __iomem *src_base)
{
	enum imx_bootsource src = bootsource_unknown;
	uint32_t reg;
	unsigned int ctrl, type;

	reg = readl(src_base + IMX51_SRC_SBMR);

	switch ((reg >> IMX51_SBMR_BMOD_SHIFT) & 0x3) {
	case 0:
	case 2:
		/* internal boot */
		ctrl = (reg >> IMX51_SBMR_BT_MEM_CTL_SHIFT) & 0x3;
		type = (reg >> IMX51_SBMR_BT_MEM_TYPE_SHIFT) & 0x3;

		src = locations[ctrl][type];
		break;
	case 1:
		/* reserved */
		src = bootsource_unknown;
		break;
	case 3:
		src = bootsource_serial;
		break;

	}

	imx_set_bootsource(src);

	return 0;
}

#define IMX53_SRC_SBMR	0x4
int imx53_boot_save_loc(void __iomem *src_base)
{
	enum imx_bootsource src = bootsource_unknown;
	uint32_t cfg1 = readl(src_base + IMX53_SRC_SBMR) & 0xff;

	switch (cfg1 >> 4) {
	case 2:
		src = bootsource_hd;
		break;
	case 3:
		if (cfg1 & (1 << 3))
			src = bootsource_spi;
		else
			src = bootsource_i2c;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		src = bootsource_mmc;
		break;
	default:
		break;
	}

	if (cfg1 & (1 << 7))
		src = bootsource_nand;

	imx_set_bootsource(src);

	return 0;
}

#define IMX6_SRC_SBMR1	0x04
#define IMX6_SRC_SBMR2	0x1c

int imx6_boot_save_loc(void __iomem *src_base)
{
	enum imx_bootsource src = bootsource_unknown;
	uint32_t sbmr2 = readl(src_base + IMX6_SRC_SBMR2) >> 24;
	uint32_t cfg1 = readl(src_base + IMX6_SRC_SBMR1) & 0xff;
	uint32_t boot_cfg_4_2_0;
	int boot_mode;

	boot_mode = (sbmr2 >> 24) & 0x3;

	switch (boot_mode) {
	case 0: /* Fuses, fall through */
	case 2: /* internal boot */
		goto internal_boot;
	case 1: /* Serial Downloader */
		src = bootsource_serial;
		break;
	case 3: /* reserved */
		break;
	};

	imx_set_bootsource(src);

	return 0;

internal_boot:

	switch (cfg1 >> 4) {
	case 2:
		src = bootsource_hd;
		break;
	case 3:
		boot_cfg_4_2_0 = (cfg1 >> 16) & 0x7;

		if (boot_cfg_4_2_0 > 4)
			src = bootsource_i2c;
		else
			src = bootsource_spi;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		src = bootsource_mmc;
		break;
	default:
		break;
	}

	if (cfg1 & (1 << 7))
		src = bootsource_nand;

	imx_set_bootsource(src);

	return 0;
}
