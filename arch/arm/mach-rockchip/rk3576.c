// SPDX-License-Identifier:     GPL-2.0+
#include <common.h>
#include <io.h>
#include <bootsource.h>
#include <mach/rockchip/rk3576-regs.h>
#include <mach/rockchip/rockchip.h>
#include <asm/barebox-arm-head.h>
#include <mach/rockchip/bootrom.h>

void rk3576_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();
}

int rk3576_init(void)
{
	rockchip_parse_bootrom_iram(rockchip_scratch_space());

	return 0;
}
