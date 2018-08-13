#ifndef __MACH_IMX8MQ_H
#define __MACH_IMX8MQ_H

#include <io.h>
#include <mach/generic.h>
#include <mach/imx8mq-regs.h>
#include <mach/revision.h>
#include <linux/bitfield.h>

#define IMX8MQ_ROM_VERSION_A0	0x800
#define IMX8MQ_ROM_VERSION_B0	0x83C

#define MX8MQ_ANATOP_DIGPROG	0x6c

#define DIGPROG_MAJOR	GENMASK(23, 8)
#define DIGPROG_MINOR	GENMASK(7, 0)

#define IMX8M_CPUTYPE_IMX8MQ	0x8240

static inline int imx8mq_cpu_revision(void)
{
	void __iomem *anatop = IOMEM(MX8MQ_ANATOP_BASE_ADDR);
	uint32_t revision = FIELD_GET(DIGPROG_MINOR,
				      readl(anatop + MX8MQ_ANATOP_DIGPROG));

	if (revision == IMX_CHIP_REV_1_0) {
		uint32_t rom_version;
		/*
		 * For B0 chip, the DIGPROG is not updated, still TO1.0.
		 * we have to check ROM version further
		 */
		rom_version = readl(IOMEM(IMX8MQ_ROM_VERSION_A0));
		if (rom_version != IMX_CHIP_REV_1_0) {
			rom_version = readl(IOMEM(IMX8MQ_ROM_VERSION_B0));
			if (rom_version >= IMX_CHIP_REV_2_0)
				revision = IMX_CHIP_REV_2_0;
		}
	}

	return revision;
}

#endif /* __MACH_IMX8_H */