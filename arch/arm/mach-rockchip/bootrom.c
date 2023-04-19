// SPDX-License-Identifier: GPL-2.0-only
#include <mach/rockchip/bootrom.h>
#include <io.h>
#include <bootsource.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <errno.h>

#define BROM_BOOTSOURCE_ID	0x10
#define BROM_BOOTSOURCE_SLOT	0x14
#define	BROM_BOOTSOURCE_SLOT_ACTIVE	GENMASK(12, 10)

static const void __iomem *rk_iram;

int rockchip_bootsource_get_active_slot(void)
{
	if (!rk_iram)
		return -EINVAL;

	return FIELD_GET(BROM_BOOTSOURCE_SLOT_ACTIVE,
			 readl(IOMEM(rk_iram) + BROM_BOOTSOURCE_SLOT));
}

struct rk_bootsource {
	enum bootsource src;
	int instance;
};

static struct rk_bootsource bootdev_map[] = {
	[0x1] = { .src = BOOTSOURCE_NAND, .instance = 0 },
	[0x2] = { .src = BOOTSOURCE_MMC, .instance = 0 },
	[0x3] = { .src = BOOTSOURCE_SPI_NOR, .instance = 0 },
	[0x4] = { .src = BOOTSOURCE_SPI_NAND, .instance = 0 },
	[0x5] = { .src = BOOTSOURCE_MMC, .instance = 1 },
	[0xa] = { .src = BOOTSOURCE_USB, .instance = 0 },
};

void rockchip_parse_bootrom_iram(const void *iram)
{
	u32 v;

	rk_iram = iram;

	v = readl(iram + BROM_BOOTSOURCE_ID);

	if (v >= ARRAY_SIZE(bootdev_map))
		return;

	bootsource_set(bootdev_map[v].src, bootdev_map[v].instance);
}
