/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <bootstrap.h>
#include <filetype.h>

void bootstrap_boot(kernel_entry_func func, bool barebox)
{
	if (!func)
		return;

	if (barebox && !is_barebox_head((void*)func))
		return;

	shutdown_barebox();
	func(0, 0, NULL);

	while (1);
}
