/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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
 */

#include <common.h>
#include <bootsource.h>
#include <command.h>
#include <complete.h>
#include <init.h>
#include <io.h>
#include <stmp-device.h>

#include <mach/generic.h>
#include <mach/imx-regs.h>
#include <mach/revision.h>

#define HW_RTC_PERSISTENT1     0x070

static int imx_reset_usb_bootstrap(void)
{
	/*
	 * Clear USB boot mode.
	 *
	 * When the i.MX28 boots from USB, the ROM code sets this bit. When
	 * after a reset the ROM code detects that this bit is set it will
	 * boot from USB again. This means that if we boot once from USB the
	 * chip will continue to boot from USB until the next power cycle.
	 *
	 * To prevent this (and boot from the configured bootsource instead)
	 * clear this bit here.
	 */
	writel(0x2, IMX_WDT_BASE + HW_RTC_PERSISTENT1 + STMP_OFFSET_REG_CLR);

	return 0;
}
device_initcall(imx_reset_usb_bootstrap);

static int __silicon_revision = SILICON_REVISION_UNKNOWN;

int silicon_revision_get(void)
{
	return __silicon_revision;
}

void silicon_revision_set(const char *soc, int revision)
{
	__silicon_revision = revision;

	printf("detected %s revision %d.%d\n", soc,
	       (revision >> 4) & 0xf, revision & 0xf);
}

#define HW_DIGCTL_CHIPID	(0x8001c310)
#define HW_DIGCTL_CHIPID_MASK	(0xffff << 16)
#define HW_DIGCTL_CHIPID_MX23	(0x3780 << 16)
#define HW_DIGCTL_CHIPID_MX28	(0x2800 << 16)

static void mxs_silicon_revision(void)
{
	enum silicon_revision revision = SILICON_REVISION_UNKNOWN;
	const char *product = "unknown";
	uint32_t reg;
	uint8_t rev;

	reg = readl((void __iomem *)HW_DIGCTL_CHIPID);
	rev = reg & 0xff;

	switch (reg & HW_DIGCTL_CHIPID_MASK) {
	case HW_DIGCTL_CHIPID_MX23:
		product = "i.MX23";
		switch (rev) {
		case 0x0: revision = SILICON_REVISION_1_0; break;
		case 0x1: revision = SILICON_REVISION_1_1; break;
		case 0x2: revision = SILICON_REVISION_1_2; break;
		case 0x3: revision = SILICON_REVISION_1_3; break;
		case 0x4: revision = SILICON_REVISION_1_4; break;
		}
		break;
	case HW_DIGCTL_CHIPID_MX28:
		product = "i.MX28";
		switch (rev) {
		case 0x1: revision = SILICON_REVISION_1_2; break;
		}
	}

	silicon_revision_set(product, revision);
}

#define HW_PINCTRL_MUXSEL2 0x120
#define HW_PINCTRL_MUXSEL3 0x130
#define HW_PINCTRL_DOE1 0x710
#define HW_PINCTRL_DIN1 0x610

/*
 * we are interested in the setting of:
 * - GPIO BANK1/19: 1 = the ROM has used the following pins for boot source selection
 * - GPIO BANK1/5: ETM enable
 * - GPIO BANK1/3: BM3
 * - GPIO BANK1/2: BM2
 * - GPIO BANK1/1: BM1
 * - GPIO BANK1/0: BM0
 */
static uint32_t mx23_detect_bootsource(void)
{
	uint32_t mux2, mux3, dir, mode;

	mux3 = readl(IMX_IOMUXC_BASE + HW_PINCTRL_MUXSEL3);
	mux2 = readl(IMX_IOMUXC_BASE + HW_PINCTRL_MUXSEL2);
	dir = readl(IMX_IOMUXC_BASE + HW_PINCTRL_DOE1);

	/* force the GPIO lines of interest to input */
	writel(0x0008002f, IMX_IOMUXC_BASE + HW_PINCTRL_DOE1 + 8);
	/* force the GPIO lines of interest to act as GPIO */
	writel(0x00000cff, IMX_IOMUXC_BASE + HW_PINCTRL_MUXSEL2 + 4);
	writel(0x000000c0, IMX_IOMUXC_BASE + HW_PINCTRL_MUXSEL3 + 4);

	/* read the bootstrapping */
	mode = readl(IMX_IOMUXC_BASE + HW_PINCTRL_DIN1) & 0x8002f;

	/* restore previous settings */
	writel(mux3, IMX_IOMUXC_BASE + HW_PINCTRL_MUXSEL3);
	writel(mux2, IMX_IOMUXC_BASE + HW_PINCTRL_MUXSEL2);
	writel(dir, IMX_IOMUXC_BASE + HW_PINCTRL_DOE1);

	if (!(mode & (1 << 19)))
		return 0xff; /* invalid marker */

	return (mode & 0xf) | ((mode & 0x20) >> 1);
}

#define MX28_REV_1_0_MODE	(0x0001a7f0)
#define MX28_REV_1_2_MODE	(0x00019bf0)

static void mxs_boot_save_loc(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = 0;
	uint32_t mode = 0xff;

	if (cpu_is_mx23()) {
		mode = mx23_detect_bootsource();
	} else if (cpu_is_mx28()) {
		enum silicon_revision rev = silicon_revision_get();

		if (rev == SILICON_REVISION_1_2)
			mode = *(uint32_t *)MX28_REV_1_2_MODE;
		else
			mode = *(uint32_t *)MX28_REV_1_0_MODE;
	}

	switch (mode & 0xf) {
	case 0x0: src = BOOTSOURCE_USB; break;		/* "USB" */
	case 0x1: src = BOOTSOURCE_I2C_EEPROM; break;	/* "I2C, master" */
	case 0x3: instance = 1;	/* fallthrough */	/* "SSP SPI #2, master, NOR" */
	case 0x2: src = BOOTSOURCE_SPI_NOR; break;	/* "SSP SPI #1, master, NOR" */
	case 0x4: src = BOOTSOURCE_NAND; break;		/* "NAND" */
	case 0x8: src = BOOTSOURCE_SPI_EEPROM; break;	/* "SSP SPI #3, master, EEPROM" */
	case 0xa: instance = 1;	/* fallthrough */	/* "SSP SD/MMC #1" */
	case 0x9: src = BOOTSOURCE_MMC; break;		/* "SSP SD/MMC #0" */
	}

	bootsource_set(src);
	bootsource_set_instance(instance);
}

static int mxs_init(void)
{
	mxs_silicon_revision();
	mxs_boot_save_loc();

	return 0;
}
postcore_initcall(mxs_init);
