/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <io.h>
#include <restart.h>
#include <init.h>

#include <mach/devices.h>
#include <mach/sysregs.h>

static void __noreturn highbank_restart_soc(struct restart_handler *rst)
{
	hingbank_set_pwr_hard_reset();
	asm("	wfi");

	hang();
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(highbank_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);

void __noreturn poweroff()
{
	shutdown_barebox();

	hingbank_set_pwr_shutdown();
	asm("	wfi");

	while(1);
}
