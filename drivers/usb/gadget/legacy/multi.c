// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * multi.c -- Multifunction Composite driver
 *
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 Nokia Corporation
 * Copyright (C) 2009 Samsung Electronics
 * Author: Michal Nazarewicz (mina86@mina86.com)
 */

#include <common.h>
#include <linux/usb/gadget-multi.h>
#include <linux/err.h>

#include "u_serial.h"

#define DRIVER_DESC		"Multifunction Composite Gadget"

/***************************** Device Descriptor ****************************/

#define MULTI_VENDOR_NUM	0x1d6b	/* Linux Foundation */
#define MULTI_PRODUCT_NUM	0x0104	/* Multifunction Composite Gadget */

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		cpu_to_le16(0x0200),

	.bDeviceClass =		USB_CLASS_MISC /* 0xEF */,
	.bDeviceSubClass =	2,
	.bDeviceProtocol =	1,
};

#define STRING_DESCRIPTION_IDX	USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "",
	[USB_GADGET_PRODUCT_IDX].s = "",
	[USB_GADGET_SERIAL_IDX].s = "",
	[STRING_DESCRIPTION_IDX].s = "Multifunction Composite Gadget",
	{  } /* end of list */
};

static struct usb_gadget_strings *dev_strings[] = {
	&(struct usb_gadget_strings){
		.language	= 0x0409,	/* en-us */
		.strings	= strings_dev,
	},
	NULL,
};

static struct usb_function_instance *fi_acm;
static struct usb_function *f_acm;
static struct usb_function_instance *fi_dfu;
static struct usb_function *f_dfu;
static struct usb_function_instance *fi_fastboot;
static struct usb_function *f_fastboot;
static struct usb_function_instance *fi_ums;
static struct usb_function *f_ums;

static struct usb_configuration config = {
	.bConfigurationValue	= 1,
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
};

struct f_multi_opts *gadget_multi_opts;

static int multi_bind_acm(struct usb_composite_dev *cdev)
{
	int ret;

	fi_acm = usb_get_function_instance("acm");
	if (IS_ERR(fi_acm)) {
		ret = PTR_ERR(fi_acm);
		fi_acm = NULL;
		return ret;
	}

	f_acm = usb_get_function(fi_acm);
	if (IS_ERR(f_acm)) {
		ret = PTR_ERR(f_acm);
		f_acm = NULL;
		return ret;
	}

	return usb_add_function(&config, f_acm);
}

static int multi_bind_dfu(struct usb_composite_dev *cdev)
{
	int ret;
	struct f_dfu_opts *opts;

	fi_dfu = usb_get_function_instance("dfu");
	if (IS_ERR(fi_dfu)) {
		ret = PTR_ERR(fi_dfu);
		fi_dfu = NULL;
		return ret;
	}

	opts = container_of(fi_dfu, struct f_dfu_opts, func_inst);
	opts->files = gadget_multi_opts->dfu_opts.files;

	f_dfu = usb_get_function(fi_dfu);
	if (IS_ERR(f_dfu)) {
		ret = PTR_ERR(f_dfu);
		f_dfu = NULL;
		return ret;
	}

	return usb_add_function(&config, f_dfu);
}

static int multi_bind_fastboot(struct usb_composite_dev *cdev)
{
	int ret;
	struct f_fastboot_opts *opts;

	fi_fastboot = usb_get_function_instance("fastboot");
	if (IS_ERR(fi_fastboot)) {
		ret = PTR_ERR(fi_fastboot);
		fi_fastboot = NULL;
		return ret;
	}

	opts = container_of(fi_fastboot, struct f_fastboot_opts, func_inst);
	opts->common = gadget_multi_opts->fastboot_opts;

	f_fastboot = usb_get_function(fi_fastboot);
	if (IS_ERR(f_fastboot)) {
		ret = PTR_ERR(f_fastboot);
		f_fastboot = NULL;
		return ret;
	}

	return usb_add_function(&config, f_fastboot);
}

static bool fastboot_has_exports(struct f_multi_opts *opts)
{
	return !file_list_empty(opts->fastboot_opts.files) ||
		opts->fastboot_opts.export_bbu;
}

static int multi_bind_ums(struct usb_composite_dev *cdev)
{
	int ret;
	struct f_ums_opts *opts;

	fi_ums = usb_get_function_instance("ums");
	if (IS_ERR(fi_ums)) {
		ret = PTR_ERR(fi_ums);
		fi_ums = NULL;
		return ret;
	}

	opts = container_of(fi_ums, struct f_ums_opts, func_inst);
	opts->files = gadget_multi_opts->ums_opts.files;

	f_ums = usb_get_function(fi_ums);
	if (IS_ERR(f_ums)) {
		ret = PTR_ERR(f_ums);
		f_ums = NULL;
		return ret;
	}

	return usb_add_function(&config, f_ums);
}

static int multi_unbind(struct usb_composite_dev *cdev)
{
	if (gadget_multi_opts->create_acm) {
		usb_put_function(f_acm);
		usb_put_function_instance(fi_acm);
	}

	if (gadget_multi_opts->ums_opts.files) {
		usb_put_function(f_ums);
		usb_put_function_instance(fi_ums);
	}

	if (gadget_multi_opts->dfu_opts.files) {
		usb_put_function(f_dfu);
		usb_put_function_instance(fi_dfu);
	}

	if (fastboot_has_exports(gadget_multi_opts)) {
		usb_put_function(f_fastboot);
		usb_put_function_instance(fi_fastboot);
	}

	return 0;
}

static int multi_bind(struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;
	int ret;

	/* allocate string IDs */
	ret = usb_string_ids_tab(cdev, strings_dev);
	if (ret < 0)
		return ret;

	if (gadget->vendor_id && gadget->product_id) {
		device_desc.idVendor = cpu_to_le16(gadget->vendor_id);
		device_desc.idProduct = cpu_to_le16(gadget->product_id);
	} else {
		device_desc.idVendor = cpu_to_le16(MULTI_VENDOR_NUM);
		device_desc.idProduct = cpu_to_le16(MULTI_PRODUCT_NUM);
	}

	strings_dev[USB_GADGET_MANUFACTURER_IDX].s = gadget->manufacturer;
	strings_dev[USB_GADGET_PRODUCT_IDX].s = gadget->productname;
	strings_dev[USB_GADGET_SERIAL_IDX].s = gadget->serialnumber;

	device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber = strings_dev[USB_GADGET_SERIAL_IDX].id;

	config.label          = strings_dev[STRING_DESCRIPTION_IDX].s;
	config.iConfiguration = strings_dev[STRING_DESCRIPTION_IDX].id;

	ret = usb_add_config_only(cdev, &config);
	if (ret)
		return ret;

	if (fastboot_has_exports(gadget_multi_opts)) {
		printf("%s: creating Fastboot function\n", __func__);
		ret = multi_bind_fastboot(cdev);
		if (ret)
			return ret;
	}

	if (gadget_multi_opts->dfu_opts.files) {
		printf("%s: creating DFU function\n", __func__);
		ret = multi_bind_dfu(cdev);
		if (ret)
			goto unbind_fastboot;
	}

	if (gadget_multi_opts->ums_opts.files) {
		printf("%s: creating USB Mass Storage function\n", __func__);
		ret = multi_bind_ums(cdev);
		if (ret)
			goto unbind_dfu;
	}

	if (gadget_multi_opts->create_acm) {
		printf("%s: creating ACM function\n", __func__);
		ret = multi_bind_acm(cdev);
		if (ret)
			goto unbind_ums;
	}

	usb_ep_autoconfig_reset(cdev->gadget);

	dev_info(&gadget->dev, DRIVER_DESC "\n");

	return 0;
unbind_ums:
	if (gadget_multi_opts->ums_opts.files)
		usb_put_function_instance(fi_ums);
unbind_dfu:
	if (gadget_multi_opts->dfu_opts.files)
		usb_put_function_instance(fi_dfu);
unbind_fastboot:
	if (fastboot_has_exports(gadget_multi_opts))
		usb_put_function_instance(fi_fastboot);

	return ret;
}

static struct usb_composite_driver multi_driver = {
	.name		= "g_multi",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_SUPER,
	.bind		= multi_bind,
	.unbind		= multi_unbind,
	.needs_serial	= 1,
};


int usb_multi_register(struct f_multi_opts *opts)
{
	int ret;

	if (gadget_multi_opts) {
		pr_err("USB multi gadget already registered\n");
		return -EBUSY;
	}

	gadget_multi_opts = opts;

	ret = usb_composite_probe(&multi_driver);
	if (ret)
		gadget_multi_opts = NULL;

	return ret;
}

void usb_multi_unregister(void)
{
	if (!gadget_multi_opts)
		return;

	usb_composite_unregister(&multi_driver);
	if (gadget_multi_opts->release)
		gadget_multi_opts->release(gadget_multi_opts);

	gadget_multi_opts = NULL;
}

unsigned usb_multi_count_functions(struct f_multi_opts *opts)
{
	unsigned count = 0;

	count += fastboot_has_exports(opts);
	count += !file_list_empty(opts->dfu_opts.files);
	count += !file_list_empty(opts->ums_opts.files);
	count += opts->create_acm;

	return count;
}

void usb_multi_opts_release(struct f_multi_opts *opts)
{
	file_list_free(opts->fastboot_opts.files);
	file_list_free(opts->dfu_opts.files);
	file_list_free(opts->ums_opts.files);

	free(opts);
}
