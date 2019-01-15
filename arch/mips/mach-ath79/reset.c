// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013 Du Huanpeng <u74147@gmail.com>
 */

#include <common.h>
#include <init.h>
#include <restart.h>
#include <mach/ath79.h>

static void __noreturn ath79_restart_soc(struct restart_handler *rst)
{
	ath79_reset_wr(AR933X_RESET_REG_RESET_MODULE, AR71XX_RESET_FULL_CHIP);
	/*
	 * Used to command a full chip reset. This is the software equivalent of
	 * pulling the reset pin. The system will reboot with PLL disabled.
	 * Always zero when read.
	 */
	hang();
	/*NOTREACHED*/
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(ath79_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
