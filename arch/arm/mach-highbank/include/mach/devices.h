/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __ASM_ARCH_DEVICES_H__
#define __ASM_ARCH_DEVICES_H__

void highbank_add_ddram(u32 size);

void highbank_register_uart(void);
void highbank_register_ahci(void);
void highbank_register_xgmac(unsigned id);
void highbank_register_gpio(unsigned id);

#endif /* __ASM_ARCH_DEVICES_H__ */
