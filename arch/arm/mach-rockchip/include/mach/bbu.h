#ifndef __MACH_ROCKCHIP_BBU_H
#define __MACH_ROCKCHIP_BBU_H

#include <bbu.h>

#ifdef CONFIG_BAREBOX_UPDATE
int rk3568_bbu_mmc_register(const char *name, unsigned long flags,
                const char *devicefile);
#else
static inline int rk3568_bbu_mmc_register(const char *name, unsigned long flags,
                const char *devicefile)
{
	return -ENOSYS;
}
#endif

# endif /* __MACH_ROCKCHIP_BBU_H */
