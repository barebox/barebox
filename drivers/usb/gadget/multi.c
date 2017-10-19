/*
 * multi.c -- Multifunction Composite driver
 *
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 Nokia Corporation
 * Copyright (C) 2009 Samsung Electronics
 * Author: Michal Nazarewicz (mina86@mina86.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <usb/gadget-multi.h>
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
	opts->files = gadget_multi_opts->fastboot_opts.files;
	opts->export_bbu = gadget_multi_opts->fastboot_opts.export_bbu;

	f_fastboot = usb_get_function(fi_fastboot);
	if (IS_ERR(f_fastboot)) {
		ret = PTR_ERR(f_fastboot);
		f_fastboot = NULL;
		return ret;
	}

	return usb_add_function(&config, f_fastboot);
}

static int multi_unbind(struct usb_composite_dev *cdev)
{
	if (gadget_multi_opts->create_acm) {
		usb_put_function(f_acm);
		usb_put_function_instance(fi_acm);
	}

	if (gadget_multi_opts->dfu_opts.files) {
		usb_put_function(f_dfu);
		usb_put_function_instance(fi_dfu);
	}

	if (gadget_multi_opts->fastboot_opts.files) {
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

	device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;

	config.label          = strings_dev[STRING_DESCRIPTION_IDX].s;
	config.iConfiguration = strings_dev[STRING_DESCRIPTION_IDX].id;

	ret = usb_add_config_only(cdev, &config);
	if (ret)
		return ret;

	if (gadget_multi_opts->fastboot_opts.files) {
		printf("%s: creating Fastboot function\n", __func__);
		ret = multi_bind_fastboot(cdev);
		if (ret)
			goto out;
	}

	if (gadget_multi_opts->dfu_opts.files) {
		printf("%s: creating DFU function\n", __func__);
		ret = multi_bind_dfu(cdev);
		if (ret)
			goto out;
	}

	if (gadget_multi_opts->create_acm) {
		printf("%s: creating ACM function\n", __func__);
		ret = multi_bind_acm(cdev);
		if (ret)
			goto out;
	}

	usb_ep_autoconfig_reset(cdev->gadget);

	dev_info(&gadget->dev, DRIVER_DESC "\n");

	return 0;
out:
	multi_unbind(cdev);

	return ret;
}

static struct usb_composite_driver multi_driver = {
	.name		= "g_multi",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
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
	if (ret) {
		usb_composite_unregister(&multi_driver);
		gadget_multi_opts = NULL;
	}

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

void usb_multi_opts_release(struct f_multi_opts *opts)
{
	if (opts->fastboot_opts.files)
		file_list_free(opts->fastboot_opts.files);
	if (opts->dfu_opts.files)
		file_list_free(opts->dfu_opts.files);

	free(opts);
}
