// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <of.h>
#include <init.h>
#include <mach/aic.h>
#include <mach/sama5d2.h>
#include <asm/cache-l2x0.h>
#include <mach/sama5_bootsource.h>
#include <asm/mmu.h>
#include <mach/cpu.h>

#define SFR_CAN		0x48
#define SFR_L2CC_HRAMC	0x58

static void sama5d2_can_ram_init(void)
{
	writel(0x00210021, SAMA5D2_BASE_SFR + SFR_CAN);
}

static void sama5d2_l2x0_init(void)
{
	void __iomem *l2x0_base = SAMA5D2_BASE_L2CC;
	u32 cfg;

	writel(0x1, SAMA5D2_BASE_SFR + SFR_L2CC_HRAMC);

	/* Prefetch Control */
	cfg = readl(l2x0_base + L2X0_PREFETCH_CTRL);
	/* prefetch offset: TODO find proper values */
	cfg |= 0x1;
	cfg |= L2X0_INCR_DOUBLE_LINEFILL_EN | L2X0_PREFETCH_DROP_EN
		| L2X0_DOUBLE_LINEFILL_EN;
	cfg |= L2X0_DATA_PREFETCH_EN | L2X0_INSTRUCTION_PREFETCH_EN;
	writel(cfg, l2x0_base + L2X0_PREFETCH_CTRL);

	/* Power Control */
	cfg = readl(l2x0_base + L2X0_POWER_CTRL);
	cfg |= L2X0_STNDBY_MODE_EN | L2X0_DYNAMIC_CLK_GATING_EN;
	writel(cfg, l2x0_base + L2X0_POWER_CTRL);

	l2x0_init(l2x0_base, 0x0, ~0UL);
}

static int sama5d2_init(void)
{
	if (!of_machine_is_compatible("atmel,sama5d2"))
		return 0;

	at91_aic_redir(SAMA5D2_BASE_SFR, SAMA5D2_AICREDIR_KEY);
	sama5d2_can_ram_init();
	sama5d2_l2x0_init();

	return 0;
}
postmmu_initcall(sama5d2_init);

static int sama5d2_bootsource_init(void)
{
	if (!of_machine_is_compatible("atmel,sama5d2"))
		return 0;

	at91_bootsource = __sama5d2_stashed_bootrom_r4;

	bootsource_set(sama5_bootsource(at91_bootsource));
	bootsource_set_instance(sama5_bootsource_instance(at91_bootsource));

	return 0;
}
postcore_initcall(sama5d2_bootsource_init);
