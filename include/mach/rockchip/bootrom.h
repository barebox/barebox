/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_ROCKCHIP_BOOTROM_H
#define __MACH_ROCKCHIP_BOOTROM_H

#include <linux/compiler.h>
#include <linux/string.h>
#include <asm/barebox-arm.h>

struct rockchip_scratch_space {
	u32 irom[16];
};

static inline void rockchip_store_bootrom_iram(ulong membase,
                                               ulong memsize,
                                               const void *iram)
{
	void *dst = (void *)arm_mem_scratch(membase + memsize);
	memcpy(dst, iram, sizeof(struct rockchip_scratch_space));
}

static inline const struct rockchip_scratch_space *rockchip_scratch_space(void)
{
	return arm_mem_scratch_get();
}

void rockchip_parse_bootrom_iram(const void *iram);

int rockchip_bootsource_get_active_slot(void);


#endif
