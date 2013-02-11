/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __ASM_ARCH_DEVICES_H__
#define __ASM_ARCH_DEVICES_H__

void vexpress_a9_add_ddram(u32 ddr0_size, u32 ddr1_size);
void vexpress_add_ddram(u32 size);

void vexpress_a9_register_uart(unsigned id);
void vexpress_register_uart(unsigned id);

void vexpress_a9_init(void);
void vexpress_init(void);

extern void *v2m_wdt_base;
extern void *v2m_sysreg_base;

#endif /* __ASM_ARCH_DEVICES_H__ */
