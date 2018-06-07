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

#include <init.h>
#include <common.h>
#include <io.h>
#include <asm/syscounter.h>
#include <asm/system.h>
#include <mach/generic.h>
#include <mach/revision.h>
#include <mach/imx8mq-regs.h>

#define IMX8MQ_ROM_VERSION_A0	0x800
#define IMX8MQ_ROM_VERSION_B0	0x83C

#define MX8MQ_ANATOP_DIGPROG	0x6c

static void imx8mq_silicon_revision(void)
{
	void __iomem *anatop = IOMEM(MX8MQ_ANATOP_BASE_ADDR);
	uint32_t reg = readl(anatop + MX8MQ_ANATOP_DIGPROG);
	uint32_t type = (reg >> 16) & 0xff;
	uint32_t rom_version;
	const char *cputypestr;

	reg &= 0xff;

	if (reg == IMX_CHIP_REV_1_0) {
		/*
		 * For B0 chip, the DIGPROG is not updated, still TO1.0.
		 * we have to check ROM version further
		 */
		rom_version = readl(IOMEM(IMX8MQ_ROM_VERSION_A0));
		if (rom_version != IMX_CHIP_REV_1_0) {
			rom_version = readl(IOMEM(IMX8MQ_ROM_VERSION_B0));
			if (rom_version >= IMX_CHIP_REV_2_0)
				reg = IMX_CHIP_REV_2_0;
		}
	}

	switch (type) {
	case 0x82:
		cputypestr = "i.MX8MQ";
		break;
	default:
		cputypestr = "unknown i.MX8M";
		break;
	};

	imx_set_silicon_revision(cputypestr, reg);
}

static int imx8mq_init_syscnt_frequency(void)
{
	void __iomem *syscnt = IOMEM(MX8MQ_SYSCNT_CTRL_BASE_ADDR);
	/*
	 * Update with accurate clock frequency
	 */
	set_cntfrq(syscnt_get_cntfrq(syscnt));
	syscnt_enable(syscnt);

	return 0;
}
/*
 * This call needs to happen before timer driver gets probed and
 * requests its update frequency via cntfrq_el0
 */
core_initcall(imx8mq_init_syscnt_frequency);

int imx8mq_init(void)
{
	imx8mq_silicon_revision();

	return 0;
}
