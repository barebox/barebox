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
#define pr_fmt(fmt) "usbgadget autostart: " fmt

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
static int fastboot_bbu;

static int usbgadget_autostart(void)
{
	struct f_multi_opts *opts;
	int ret;

	if (!autostart)
		return 0;

	opts = xzalloc(sizeof(*opts));
	opts->release = usb_multi_opts_release;

	if (fastboot_function) {
		opts->fastboot_opts.files = file_list_parse(fastboot_function);
		if (IS_ERR(opts->fastboot_opts.files)) {
			pr_err("Parsing file list \"%s\" failed: %s\n", fastboot_function,
			       strerrorp(opts->fastboot_opts.files));
			opts->fastboot_opts.files = NULL;
		}

		opts->fastboot_opts.export_bbu = fastboot_bbu;
	}

	opts->create_acm = acm;

	if (!opts->fastboot_opts.files && !opts->create_acm) {
		pr_warn("No functions to register\n");
		return 0;
	}

	setenv("otg.mode", "peripheral");

	ret = usb_multi_register(opts);
	if (ret)
		usb_multi_opts_release(opts);

	return ret;
}
postenvironment_initcall(usbgadget_autostart);

static int usbgadget_globalvars_init(void)
{

	globalvar_add_simple_bool("usbgadget.autostart", &autostart);
	globalvar_add_simple_bool("usbgadget.acm", &acm);
	globalvar_add_simple_string("usbgadget.fastboot_function",
				    &fastboot_function);
	globalvar_add_simple_bool("usbgadget.fastboot_bbu", &fastboot_bbu);

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
BAREBOX_MAGICVAR_NAMED(global_usbgadget_fastboot_bbu,
		       global.usbgadget.fastboot_bbu,
		       "usbgadget: export barebox update handlers via fastboot");
