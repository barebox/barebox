/*
 * Copyright (c) 2017 Oleksij Rempel <o.rempel@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <command.h>
#include <errno.h>
#include <environment.h>
#include <malloc.h>
#include <getopt.h>
#include <fs.h>
#include <xfuncs.h>
#include <usb/usbserial.h>
#include <usb/dfu.h>
#include <usb/gadget-multi.h>
#include <globalvar.h>
#include <magicvar.h>

static int autostart;
static int acm;
static char *fastboot_function;

static int usbgadget_autostart(void)
{
	struct f_multi_opts opts = {};

	if (!autostart)
		return 0;

	setenv("otg.mode", "peripheral");

	if (fastboot_function)
		opts.fastboot_opts.files = file_list_parse(fastboot_function);

	opts.create_acm = acm;

	return usb_multi_register(&opts);
}
postenvironment_initcall(usbgadget_autostart);

static int usbgadget_globalvars_init(void)
{

	globalvar_add_simple_bool("usbgadget.autostart", &autostart);
	globalvar_add_simple_bool("usbgadget.acm", &acm);
	globalvar_add_simple_string("usbgadget.fastboot_function",
				    &fastboot_function);

	return 0;
}
device_initcall(usbgadget_globalvars_init);

BAREBOX_MAGICVAR_NAMED(global_usbgadget_autostart,
		       global.usbgadget.autostart,
		       "usbgadget: Automatically start usbgadget on boot");
BAREBOX_MAGICVAR_NAMED(global_usbgadget_acm,
		       global.usbgadget.acm,
		       "usbgadget: Create CDC ACM function");
BAREBOX_MAGICVAR_NAMED(global_usbgadget_fastboot_function,
		       global.usbgadget.fastboot_function,
		       "usbgadget: Create Android Fastboot function");
