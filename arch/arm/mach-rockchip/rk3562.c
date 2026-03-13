// SPDX-License-Identifier:     GPL-2.0+
#include <common.h>
#include <io.h>
#include <bootsource.h>
#include <reset_source.h>
#include <mach/rockchip/rk3562-regs.h>
#include <mach/rockchip/rockchip.h>
#include <asm/barebox-arm-head.h>
#include <mach/rockchip/bootrom.h>

void rk3562_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();
}

static void rk3562_detect_reset_reason(void)
{
	u32 val = readl(RK3562_CRU_BASE + 0x0620);

	switch (val) {
	case 0:
		reset_source_set(RESET_POR);
		break;
	case (1 << 0):
	case (1 << 1):
		reset_source_set(RESET_RST);
		break;
	case (1 << 2):
	case (1 << 3):
		reset_source_set(RESET_THERM);
		break;
	case (1 << 4):
	case (1 << 5):
		reset_source_set(RESET_WDG);
		break;
	default:
		reset_source_set(RESET_UKWN);
		break;
	}

	/*
	 * Clear the reset status register here so current reset status bit
	 * doesn't interfere with the check on the next boot.
	 */
	writel(0, RK3562_CRU_BASE + 0x0620);
}

int rk3562_init(void)
{
	rockchip_parse_bootrom_iram(rockchip_scratch_space()->iram);
	rk3562_detect_reset_reason();

	return 0;
}
