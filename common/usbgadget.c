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
#define pr_fmt(fmt) "usbgadget: " fmt

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
static char *dfu_function;

static struct file_list *parse(const char *files)
{
	struct file_list *list = file_list_parse(files);
	if (IS_ERR(list)) {
		pr_err("Parsing file list \"%s\" failed: %pe\n", files, list);
		return NULL;
	}
	return list;
}

int usbgadget_register(bool dfu, const char *dfu_opts,
		       bool fastboot, const char *fastboot_opts,
		       bool acm, bool export_bbu)
{
	int ret;
	struct device_d *dev;
	struct f_multi_opts *opts;
	const char *fastboot_partitions = get_fastboot_partitions();

	if (dfu && !dfu_opts && dfu_function && *dfu_function)
		dfu_opts = dfu_function;

	if (IS_ENABLED(CONFIG_FASTBOOT_BASE) && fastboot && !fastboot_opts &&
	    fastboot_partitions && *fastboot_partitions)
		fastboot_opts = fastboot_partitions;

	if (!dfu_opts && !fastboot_opts && !acm)
		return COMMAND_ERROR_USAGE;

	/*
	 * Creating a gadget with both DFU and Fastboot doesn't work.
	 * Both client tools seem to assume that the device only has
	 * a single configuration
	 */
	if (fastboot_opts && dfu_opts) {
		pr_err("Only one of Fastboot and DFU allowed\n");
		return -EINVAL;
	}

	opts = xzalloc(sizeof(*opts));
	opts->release = usb_multi_opts_release;

	if (fastboot_opts) {
		opts->fastboot_opts.files = parse(fastboot_opts);
		opts->fastboot_opts.export_bbu = export_bbu;
	}

	if (dfu_opts)
		opts->dfu_opts.files = parse(dfu_opts);

	if (!opts->dfu_opts.files && !opts->fastboot_opts.files && !acm) {
		pr_warn("No functions to register\n");
		free(opts);
		return 0;
	}

	opts->create_acm = acm;

	dev = get_device_by_name("otg");
	if (dev)
		dev_set_param(dev, "mode", "peripheral");

	ret = usb_multi_register(opts);
	if (ret)
		usb_multi_opts_release(opts);

	return ret;
}

static int usbgadget_autostart(void)
{
	bool fastboot_bbu = get_fastboot_bbu();

	if (!IS_ENABLED(CONFIG_USB_GADGET_AUTOSTART) || !autostart)
		return 0;

	return usbgadget_register(true, NULL, true, NULL, acm, fastboot_bbu);
}
postenvironment_initcall(usbgadget_autostart);

static int usbgadget_globalvars_init(void)
{
	if (IS_ENABLED(CONFIG_USB_GADGET_AUTOSTART)) {
		globalvar_add_simple_bool("usbgadget.autostart", &autostart);
		globalvar_add_simple_bool("usbgadget.acm", &acm);
	}
	globalvar_add_simple_string("usbgadget.dfu_function", &dfu_function);

	return 0;
}
device_initcall(usbgadget_globalvars_init);

BAREBOX_MAGICVAR(global.usbgadget.autostart,
		 "usbgadget: Automatically start usbgadget on boot");
BAREBOX_MAGICVAR(global.usbgadget.acm,
		 "usbgadget: Create CDC ACM function");
BAREBOX_MAGICVAR(global.usbgadget.dfu_function,
		 "usbgadget: Create DFU function");
