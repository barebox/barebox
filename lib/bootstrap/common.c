/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <bootstrap.h>
#include <filetype.h>

void bootstrap_boot(int (*func)(void), bool barebox)
{
	if (!func)
		return;

	if (barebox && !is_barebox_head((void*)func))
		return;

	shutdown_barebox();
	func();

	while (1);
}
