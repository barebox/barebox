/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <io.h>

#include <mach/devices.h>
#include <mach/sysregs.h>

void __noreturn reset_cpu(ulong addr)
{
	hingbank_set_pwr_hard_reset();
	asm("	wfi");

	while(1);
}

void __noreturn poweroff()
{
	shutdown_barebox();

	hingbank_set_pwr_shutdown();
	asm("	wfi");

	while(1);
}
