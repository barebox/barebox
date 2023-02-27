/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __ASM_ARCH_DEVICES_H__
#define __ASM_ARCH_DEVICES_H__

#include <linux/amba/mmci.h>

void vexpress_a9_legacy_init(void);
void vexpress_init(void);

extern void *v2m_wdt_base;
extern void *v2m_sysreg_base;

#endif /* __ASM_ARCH_DEVICES_H__ */
