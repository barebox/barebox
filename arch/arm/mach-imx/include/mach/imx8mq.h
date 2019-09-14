#ifndef __MACH_IMX8MQ_H
#define __MACH_IMX8MQ_H

#include <io.h>
#include <mach/generic.h>
#include <mach/imx8mq-regs.h>
#include <mach/revision.h>
#include <linux/bitfield.h>

#define IMX8MQ_ROM_VERSION_A0	0x800
#define IMX8MQ_ROM_VERSION_B0	0x83C
#define IMX8MQ_OCOTP_VERSION_B1 0x40
#define IMX8MQ_OCOTP_VERSION_B1_MAGIC	0xff0055aa

#define MX8MQ_ANATOP_DIGPROG	0x6c

#define DIGPROG_MAJOR	GENMASK(23, 8)
#define DIGPROG_MINOR	GENMASK(7, 0)

#define IMX8M_CPUTYPE_IMX8MQ	0x8240

static inline int imx8mq_cpu_revision(void)
{
	void __iomem *anatop = IOMEM(MX8MQ_ANATOP_BASE_ADDR);
	void __iomem *ocotp = IOMEM(MX8MQ_OCOTP_BASE_ADDR);
	uint32_t revision = FIELD_GET(DIGPROG_MINOR,
				      readl(anatop + MX8MQ_ANATOP_DIGPROG));
	uint32_t rom_version;

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
	rom_version = readb(IOMEM(IMX8MQ_ROM_VERSION_A0));
	if (rom_version != IMX_CHIP_REV_1_0) {
		rom_version = readb(IOMEM(IMX8MQ_ROM_VERSION_B0));
		if (rom_version >= IMX_CHIP_REV_2_0)
			revision = IMX_CHIP_REV_2_0;
	}

	return revision;
}

u64 imx8mq_uid(void);

#endif /* __MACH_IMX8_H */