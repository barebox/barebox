/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX8MQ_H
#define __MACH_IMX8MQ_H

#include <io.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx8mq-regs.h>
#include <mach/imx/imx8mm-regs.h>
#include <mach/imx/imx8mn-regs.h>
#include <mach/imx/imx8mp-regs.h>
#include <mach/imx/revision.h>
#include <linux/bitfield.h>

#define IMX8MQ_ROM_VERSION_A0	0x800
#define IMX8MQ_ROM_VERSION_B0	0x83C
#define IMX8MQ_OCOTP_VERSION_B1 0x40
#define IMX8MQ_OCOTP_VERSION_B1_MAGIC	0xff0055aa

#define MX8MQ_ANATOP_DIGPROG	0x6c
#define MX8MM_ANATOP_DIGPROG	0x800
#define MX8MN_ANATOP_DIGPROG	0x800
#define MX8MP_ANATOP_DIGPROG	0x800

#define DIGPROG_MAJOR	GENMASK(23, 8)
#define DIGPROG_MINOR	GENMASK(7, 0)

#define IMX8M_CPUTYPE_IMX8MQ	0x8240
#define IMX8M_CPUTYPE_IMX8MM	0x8241
#define IMX8M_CPUTYPE_IMX8MN	0x8242
#define IMX8M_CPUTYPE_IMX8MP	0x8243

static inline int imx8mm_cpu_revision(void)
{
	void __iomem *anatop = IOMEM(MX8MM_ANATOP_BASE_ADDR);
	uint32_t revision = FIELD_GET(DIGPROG_MINOR,
				      readl(anatop + MX8MM_ANATOP_DIGPROG));
	return revision;
}

static inline int imx8mn_cpu_revision(void)
{
	void __iomem *anatop = IOMEM(MX8MN_ANATOP_BASE_ADDR);
	uint32_t revision = FIELD_GET(DIGPROG_MINOR,
				      readl(anatop + MX8MN_ANATOP_DIGPROG));
	return revision;
}

static inline int imx8mp_cpu_revision(void)
{
	void __iomem *anatop = IOMEM(MX8MP_ANATOP_BASE_ADDR);
	uint32_t revision = FIELD_GET(DIGPROG_MINOR,
				      readl(anatop + MX8MP_ANATOP_DIGPROG));
	return revision;
}

static inline int imx8mq_cpu_revision(void)
{
	void __iomem *anatop = IOMEM(MX8MQ_ANATOP_BASE_ADDR);
	void __iomem *ocotp = IOMEM(MX8MQ_OCOTP_BASE_ADDR);
	void __iomem *rom = IOMEM(0x0);
	uint32_t revision = FIELD_GET(DIGPROG_MINOR,
				      readl(anatop + MX8MQ_ANATOP_DIGPROG));
	uint32_t rom_version;

	OPTIMIZER_HIDE_VAR(rom);

	if (revision != IMX_CHIP_REV_1_0)
		return revision;
	/*
	 * For B1 chip we need to check OCOTP
	 */
	if (readl(ocotp + IMX8MQ_OCOTP_VERSION_B1) ==
	    IMX8MQ_OCOTP_VERSION_B1_MAGIC)
		return IMX_CHIP_REV_2_1;
	/*
	 * For B0 chip, the DIGPROG is not updated, still TO1.0.
	 * we have to check ROM version further
	 */
	rom_version = readb(IOMEM(rom + IMX8MQ_ROM_VERSION_A0));
	if (rom_version != IMX_CHIP_REV_1_0) {
		rom_version = readb(IOMEM(rom + IMX8MQ_ROM_VERSION_B0));
		if (rom_version >= IMX_CHIP_REV_2_0)
			revision = IMX_CHIP_REV_2_0;
	}

	return revision;
}

#endif /* __MACH_IMX8_H */
