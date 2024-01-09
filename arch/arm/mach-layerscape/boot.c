// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <init.h>
#include <bootsource.h>
#include <linux/bitfield.h>
#include <mach/layerscape/layerscape.h>
#include <soc/fsl/immap_lsch2.h>
#include <soc/fsl/immap_lsch3.h>

enum bootsource ls1046a_bootsource_get(void)
{
	void __iomem *dcfg = IOMEM(LSCH2_DCFG_ADDR);
	uint32_t rcw_src;

	rcw_src = in_be32(dcfg) >> 23;

	if (rcw_src == 0x40)
		return BOOTSOURCE_MMC;
	if ((rcw_src & 0x1fe) == 0x44)
		return BOOTSOURCE_SPI_NOR;
	if ((rcw_src & 0x1f0) == 0x10)
		/* 8bit NOR Flash */
		return BOOTSOURCE_NOR;
	if ((rcw_src & 0x1f0) == 0x20)
		/* 16bit NOR Flash */
		return BOOTSOURCE_NOR;

	return BOOTSOURCE_UNKNOWN;
}

enum bootsource ls1021a_bootsource_get(void)
{
	return ls1046a_bootsource_get();
}

void ls1021a_bootsource_init(void)
{
	bootsource_set_raw(ls1021a_bootsource_get(), BOOTSOURCE_INSTANCE_UNKNOWN);
}

void ls1046a_bootsource_init(void)
{
	bootsource_set_raw(ls1046a_bootsource_get(), BOOTSOURCE_INSTANCE_UNKNOWN);
}

#define PORSR1_RCW_SRC	GENMASK(26, 23)

static enum bootsource ls1028a_bootsource_get(int *instance)
{
	void __iomem *porsr1 = IOMEM(LSCH3_DCFG_BASE);
	uint32_t rcw_src;

	rcw_src = FIELD_GET(PORSR1_RCW_SRC, readl(porsr1));

	printf("%s: 0x%08x\n", __func__, rcw_src);

	switch (rcw_src) {
	case 8:
		*instance = 0;
		return BOOTSOURCE_MMC;
	case 9:
		*instance = 1;
		return BOOTSOURCE_MMC;
	case 0xa:
		return BOOTSOURCE_I2C;
	case 0xd:
	case 0xc:
		return BOOTSOURCE_NAND;
	case 0xf:
		return BOOTSOURCE_SPI_NOR;
	}

	return BOOTSOURCE_UNKNOWN;
}

void ls1028a_bootsource_init(void)
{
	int instance = BOOTSOURCE_INSTANCE_UNKNOWN;
	enum bootsource source = ls1028a_bootsource_get(&instance);

	bootsource_set_raw(source, instance);
}
