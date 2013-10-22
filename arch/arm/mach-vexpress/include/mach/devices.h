/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __ASM_ARCH_DEVICES_H__
#define __ASM_ARCH_DEVICES_H__

#include <linux/amba/mmci.h>

void vexpress_a9_legacy_add_ddram(u32 ddr0_size, u32 ddr1_size);
void vexpress_add_ddram(u32 size);

void vexpress_a9_legacy_register_uart(unsigned id);
void vexpress_register_uart(unsigned id);

void vexpress_a9_legacy_init(void);
void vexpress_init(void);

void vexpress_a9_legacy_register_mmc(struct mmci_platform_data *plat);
void vexpress_register_mmc(struct mmci_platform_data *plat);

extern void *v2m_wdt_base;
extern void *v2m_sysreg_base;

#endif /* __ASM_ARCH_DEVICES_H__ */
