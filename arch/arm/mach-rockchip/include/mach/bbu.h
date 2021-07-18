#ifndef __MACH_ROCKCHIP_BBU_H
#define __MACH_ROCKCHIP_BBU_H

#include <bbu.h>

static inline int rk3568_bbu_mmc_register(const char *name, unsigned long flags,
                const char *devicefile)
{
	return bbu_register_std_file_update(name, flags,
                devicefile, filetype_rockchip_rkns_image);

}

# endif /* __MACH_ROCKCHIP_BBU_H */
