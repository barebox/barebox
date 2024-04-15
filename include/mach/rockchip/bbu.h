/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_ROCKCHIP_BBU_H
#define __MACH_ROCKCHIP_BBU_H

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE
int rockchip_bbu_mmc_register(const char *name, unsigned long flags,
                const char *devicefile);
#else
static inline int rockchip_bbu_mmc_register(const char *name, unsigned long flags,
                const char *devicefile)
{
	return -ENOSYS;
}
#endif

#define rk3568_bbu_mmc_register rockchip_bbu_mmc_register

# endif /* __MACH_ROCKCHIP_BBU_H */
