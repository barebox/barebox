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
	writel(0x2, IMX_WDT_BASE + HW_RTC_PERSISTENT1 + BIT_CLR);

	return 0;
}
device_initcall(imx_reset_usb_bootstrap);

extern void imx_dump_clocks(void);

static int do_clocks(int argc, char *argv[])
{
	imx_dump_clocks();

	return 0;
}

BAREBOX_CMD_START(dump_clocks)
	.cmd		= do_clocks,
	.usage		= "show clock frequencies",
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END


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
	case HW_DIGCTL_CHIPID_MX28:
		product = "i.MX28";
		switch (rev) {
		case 0x1: revision = SILICON_REVISION_1_2; break;
		}
	}

	silicon_revision_set(product, revision);
}

#define MX28_REV_1_0_MODE	(0x0001a7f0)
#define MX28_REV_1_2_MODE	(0x00019bf0)

static void mxs_boot_save_loc(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = 0;
	uint32_t mode = 0xff;

	if (cpu_is_mx23()) {
		/* not implemented yet */
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
