// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <filetype.h>
#include <mach/omap/generic.h>
#include <mach/omap/xload.h>
#include <mach/omap/am33xx-silicon.h>
#include <mach/omap/am33xx-generic.h>
#include <mach/omap/am33xx-clock.h>
#include <bootsource.h>
#include <linux/sizes.h>
#include <asm/cache.h>
#include <pbl/bio.h>

struct xload_instance {
	void __iomem *base;
};

static void omap_hsmmc_fat_start_image(struct pbl_bio *bio, void *buf)
{
	int ret;

	ret = pbl_fat_load(bio, "barebox.bin", buf, SZ_2M);
	if (ret < 0) {
		pr_err("pbl_fat_load: error %d\n", ret);
		return;
	}

	sync_caches_for_execution();

	asm volatile ("bx  %0\n" : : "r"(buf) :);
	__builtin_unreachable();
}

static const struct xload_instance am35xx_hsmmc_instances[] = {
	[0] = { .base = IOMEM(AM33XX_MMCHS0_BASE), },
	[1] = { .base = IOMEM(AM33XX_MMC1_BASE), },
	[2] = { .base = IOMEM(AM33XX_MMCHS2_BASE), },
};

void __noreturn am33xx_hsmmc_start_image(void)
{
	void *buf = (void *)OMAP_DRAM_ADDR_SPACE_START;
	const struct xload_instance *instance;
	enum bootsource src;
	struct pbl_bio bio;
	int id = 0, ret;

	omap_dmtimer_init(IOMEM(AM33XX_DMTIMER0_BASE),
			  am33xx_get_osc_clock() * 1000);

	src = am33xx_get_bootsource(&id);
	if (src != BOOTSOURCE_MMC)
		panic("This MLO was configured only for SD/MMC\n");

	instance = &am35xx_hsmmc_instances[id];

	ret = omap_hsmmc_bio_init(&bio, instance->base, 0x100);
	if (ret)
		goto out_panic;

	omap_hsmmc_fat_start_image(&bio, buf);

out_panic:
	panic("FAT chainloading failed\n");
}
