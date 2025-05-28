/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_ROCKCHIP_BOOTROM_H
#define __MACH_ROCKCHIP_BOOTROM_H

#include <linux/compiler.h>
#include <linux/string.h>
#include <pbl.h>
#include <asm/barebox-arm.h>
#include <tee/optee.h>
#include <pbl.h>

struct rockchip_scratch_space {
	u32 irom[16];
	struct optee_header optee_hdr;
	/* FDT needs an 8 byte alignment */
	u8 fdt[CONFIG_ARCH_ROCKCHIP_ATF_FDT_SIZE] __aligned(8);
};
static_assert(sizeof(struct rockchip_scratch_space) <= CONFIG_SCRATCH_SIZE);

extern struct rockchip_scratch_space *rk_scratch;

static inline void rockchip_store_bootrom_iram(const void *iram)
{
	if (rk_scratch)
		memcpy(rk_scratch, iram, sizeof(struct rockchip_scratch_space));
}

static inline const struct rockchip_scratch_space *rockchip_scratch_space(void)
{
	return IN_PBL ? rk_scratch : arm_mem_scratch_get();
}

void rockchip_parse_bootrom_iram(const void *iram);

int rockchip_bootsource_get_active_slot(void);

static inline const struct optee_header *rk_scratch_get_optee_hdr(void)
{
	struct rockchip_scratch_space *scratch;

	if (IN_PBL)
		scratch = rk_scratch;
	else
		scratch = (void *)arm_mem_scratch_get();

	if (!scratch)
		return ERR_PTR(-EINVAL);

	return &scratch->optee_hdr;
}

#endif
