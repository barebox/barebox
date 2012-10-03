/*
 * (C) 2007 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * based on existing SAM7DFU code from OpenPCD:
 * (C) Copyright 2006 by Harald Welte <hwelte@hmw-consulting.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * TODO:
 * - make NAND support reasonably self-contained and put in apropriate
 *   ifdefs
 * - add some means of synchronization, i.e. block commandline access
 *   while DFU transfer is in progress, and return to commandline once
 *   we're finished
 * - add VERIFY support after writing to flash
 * - sanely free() resources allocated during first uppload/download
 *   request when aborting
 * - sanely free resources when another alternate interface is selected
 *
 * Maybe:
 * - add something like uImage or some other header that provides CRC
 *   checking?
 * - make 'dnstate' attached to 'struct usb_device_instance'
 */

#include <dma.h>
#include <asm/byteorder.h>
#include <usb/composite.h>
#include <linux/types.h>
#include <linux/list.h>
#include <usb/gadget.h>
#include <linux/stat.h>
#include <usb/ch9.h>
#include <usb/dfu.h>
#include <config.h>
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <libbb.h>
#include <init.h>
#include <fs.h>

#define USB_DT_DFU_SIZE			9
#define USB_DT_DFU			0x21

#define CONFIG_USBD_DFU_XFER_SIZE     4096
#define DFU_TEMPFILE "/dfu_temp"

static int dfualt;
static int dfufd = -EINVAL;;
static struct usb_dfu_dev *dfu_devs;
static int dfu_num_alt;
static int dfudetach;

/* USB DFU functional descriptor */
static struct usb_dfu_func_descriptor usb_dfu_func = {
	.bLength		= USB_DT_DFU_SIZE,
	.bDescriptorType	= USB_DT_DFU,
	.bmAttributes		= USB_DFU_CAN_UPLOAD | USB_DFU_CAN_DOWNLOAD | USB_DFU_MANIFEST_TOL,
	.wDetachTimeOut		= 0xff00,
	.wTransferSize		= CONFIG_USBD_DFU_XFER_SIZE,
	.bcdDFUVersion		= 0x0100,
};

struct f_dfu {
	struct usb_function		func;
	u8				port_num;

	u8	dfu_state;
	u8	dfu_status;
	struct usb_request		*dnreq;
};

static inline struct f_dfu *func_to_dfu(struct usb_function *f)
{
	return container_of(f, struct f_dfu, func);
}

/* static strings, in UTF-8 */
static struct usb_string *dfu_string_defs;

static struct usb_gadget_strings dfu_string_table = {
	.language =		0x0409,	/* en-us */
};

static struct usb_gadget_strings *dfu_strings[] = {
	&dfu_string_table,
	NULL,
};

static struct usb_interface_descriptor dfu_control_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	0,
	.bInterfaceClass =	0xfe,
	.bInterfaceSubClass =	1,
	.bInterfaceProtocol =	1,
	/* .iInterface = DYNAMIC */
};

static int
dfu_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_descriptor_header **header;
	struct usb_interface_descriptor *desc;
	int i;
	int			status;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		return status;

	dfu_control_interface_desc.bInterfaceNumber = status;

	header = xzalloc(sizeof(void *) * (dfu_num_alt + 2));
	desc = xzalloc(sizeof(struct usb_interface_descriptor) * dfu_num_alt);
	for (i = 0; i < dfu_num_alt; i++) {
		desc[i].bLength =		USB_DT_INTERFACE_SIZE;
		desc[i].bDescriptorType =	USB_DT_INTERFACE;
		desc[i].bNumEndpoints	=	0;
		desc[i].bInterfaceClass =	0xfe;
		desc[i].bInterfaceSubClass =	1;
		desc[i].bInterfaceProtocol =	1;
		desc[i].bAlternateSetting = i;
		desc[i].iInterface = dfu_string_defs[i + 1].id;
		header[i] = (struct usb_descriptor_header *)&desc[i];
	}
	header[i] = (struct usb_descriptor_header *) &usb_dfu_func;
	header[i + 1] = NULL;

	/* copy descriptors, and track endpoint copies */
	f->descriptors = usb_copy_descriptors(header);
	if (!f->descriptors)
		goto out;

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(header);
	}

	for (i = 0; i < dfu_num_alt; i++)
		printf("dfu: register alt%d(%s) with device %s\n",
				i, dfu_devs[i].name, dfu_devs[i].dev);
out:
	free(desc);
	free(header);

	if (status)
		ERROR(cdev, "%s/%p: can't bind, err %d\n", f->name, f, status);

	return status;
}

static void
dfu_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_dfu		*dfu = func_to_dfu(f);

	free(f->descriptors);
	if (gadget_is_dualspeed(c->cdev->gadget))
		free(f->hs_descriptors);

	dma_free(dfu->dnreq->buf);
	usb_ep_free_request(c->cdev->gadget->ep0, dfu->dnreq);
	free(dfu);
}

static int dfu_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	dfualt = alt;

	return 0;
}

static int dfu_status(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	struct dfu_status *dstat = (struct dfu_status *) req->buf;

	dstat->bStatus = dfu->dfu_status;
	dstat->bState  = dfu->dfu_state;
	dstat->iString = 0;
	dstat->bwPollTimeout[0] = 10;
	dstat->bwPollTimeout[1] = 0;
	dstat->bwPollTimeout[2] = 0;

	return sizeof(*dstat);
}

static void dfu_cleanup(struct f_dfu *dfu)
{
	struct stat s;

	if (dfufd > 0) {
		close(dfufd);
		dfufd = -EINVAL;
	}

	if (!stat(DFU_TEMPFILE, &s))
		unlink(DFU_TEMPFILE);
}

static void dn_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_dfu		*dfu = req->context;
	int ret;

	ret = write(dfufd, req->buf, req->length);
	if (ret < (int)req->length) {
		perror("write");
		dfu->dfu_status = DFU_STATUS_errWRITE;
		dfu_cleanup(dfu);
	}
}

static int handle_dnload(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	u16			w_length = le16_to_cpu(ctrl->wLength);
	int ret;

	if (w_length == 0) {
		dfu->dfu_state = DFU_STATE_dfuIDLE;
		if (dfu_devs[dfualt].flags & DFU_FLAG_SAVE) {
			int fd;

			fd = open(dfu_devs[dfualt].dev, O_WRONLY);
			if (fd < 0) {
				perror("open");
				ret = -EINVAL;
				goto err_out;
			}
			ret = erase(fd, ~0, 0);
			close(fd);
			if (ret && ret != -ENOSYS) {
				perror("erase");
				ret = -EINVAL;
				goto err_out;
			}
			ret = copy_file(DFU_TEMPFILE, dfu_devs[dfualt].dev, 0);
			if (ret) {
				printf("copy file failed\n");
				ret = -EINVAL;
				goto err_out;
			}
		}
		dfu_cleanup(dfu);
		return 0;
	}

	dfu->dnreq->length = w_length;
	dfu->dnreq->context = dfu;
	usb_ep_queue(cdev->gadget->ep0, dfu->dnreq);

	return 0;

err_out:
	dfu_cleanup(dfu);
	return ret;
}

static void up_complete(struct usb_ep *ep, struct usb_request *req)
{
}

static int handle_upload(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	u16			w_length = le16_to_cpu(ctrl->wLength);
	int len;

	len = read(dfufd, dfu->dnreq->buf, w_length);

	dfu->dnreq->length = len;
	if (len < w_length) {
		dfu_cleanup(dfu);
		dfu->dfu_state = DFU_STATE_dfuIDLE;
	}

	dfu->dnreq->complete = up_complete;
	usb_ep_queue(cdev->gadget->ep0, dfu->dnreq);

	return 0;
}

static void dfu_abort(struct f_dfu *dfu)
{
	dfu->dfu_state = DFU_STATE_dfuIDLE;
	dfu->dfu_status = DFU_STATUS_OK;

	dfu_cleanup(dfu);
}

static int dfu_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	int			w_length = le16_to_cpu(ctrl->wLength);
	int			w_value = le16_to_cpu(ctrl->wValue);
	int ret;

	if (ctrl->bRequestType == USB_DIR_IN && ctrl->bRequest == USB_REQ_GET_DESCRIPTOR
			&& (w_value >> 8) == 0x21) {
		value = min(w_length, (int)sizeof(usb_dfu_func));
		memcpy(req->buf, &usb_dfu_func, value);
		goto out;
	}

	/* Allow GETSTATUS in every state */
	if (ctrl->bRequest == USB_REQ_DFU_GETSTATUS) {
		value = dfu_status(f, ctrl);
		value = min(value, w_length);
		goto out;
	}

	/* Allow GETSTATE in every state */
	if (ctrl->bRequest == USB_REQ_DFU_GETSTATE) {
		*(u8 *)req->buf = dfu->dfu_state;
		value = sizeof(u8);
		goto out;
	}

	switch (dfu->dfu_state) {
	case DFU_STATE_appIDLE:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_DETACH:
			dfu->dfu_state = DFU_STATE_appDETACH;
			value = 0;
			goto out;
			break;
		default:
			value = -EINVAL;
		}
		break;
	case DFU_STATE_appDETACH:
		switch (ctrl->bRequest) {
		default:
			dfu->dfu_state = DFU_STATE_appIDLE;
			value = -EINVAL;
			goto out;
			break;
		}
		break;
	case DFU_STATE_dfuIDLE:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_DNLOAD:
			if (w_length == 0) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				value = -EINVAL;
				goto out;
			}
			debug("dfu: starting download to %s\n", dfu_devs[dfualt].dev);
			if (dfu_devs[dfualt].flags & DFU_FLAG_SAVE)
				dfufd = open(DFU_TEMPFILE, O_WRONLY | O_CREAT);
			else
				dfufd = open(dfu_devs[dfualt].dev, O_WRONLY);

			if (dfufd < 0) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				perror("open");
				goto out;
			}

			ret = erase(dfufd, ~0, 0);
			if (ret && ret != -ENOSYS) {
				dfu->dfu_status = DFU_STATUS_errERASE;
				perror("erase");
				goto out;
			}

			value = handle_dnload(f, ctrl);
			dfu->dfu_state = DFU_STATE_dfuDNLOAD_IDLE;
			return 0;
			break;
		case USB_REQ_DFU_UPLOAD:
			dfu->dfu_state = DFU_STATE_dfuUPLOAD_IDLE;
			debug("dfu: starting upload from %s\n", dfu_devs[dfualt].dev);
			if (!(dfu_devs[dfualt].flags & DFU_FLAG_READBACK)) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				goto out;
			}
			dfufd = open(dfu_devs[dfualt].dev, O_RDONLY);
			if (dfufd < 0) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				perror("open");
				goto out;
			}
			handle_upload(f, ctrl);
			return 0;
			break;
		case USB_REQ_DFU_ABORT:
			dfu->dfu_status = DFU_STATUS_OK;
			value = 0;
			break;
		case USB_REQ_DFU_DETACH:
			/* Proprietary extension: 'detach' from idle mode and
			 * get back to runtime mode in case of USB Reset.  As
			 * much as I dislike this, we just can't use every USB
			 * bus reset to switch back to runtime mode, since at
			 * least the Linux USB stack likes to send a number of resets
			 * in a row :( */
			dfu->dfu_state = DFU_STATE_dfuMANIFEST_WAIT_RST;
			value = 0;
			dfudetach = 1;
			break;
		default:
			dfu->dfu_state = DFU_STATE_dfuERROR;
			value = -EINVAL;
			goto out;
			break;
		}
		break;
	case DFU_STATE_dfuDNLOAD_IDLE:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_DNLOAD:
			value = handle_dnload(f, ctrl);
			if (dfu->dfu_state != DFU_STATE_dfuIDLE) {
				return 0;
			}
			break;
		case USB_REQ_DFU_ABORT:
			dfu_abort(dfu);
			value = 0;
			break;
		default:
			dfu->dfu_state = DFU_STATE_dfuERROR;
			value = -EINVAL;
			break;
		}
		break;
	case DFU_STATE_dfuUPLOAD_IDLE:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_UPLOAD:
			handle_upload(f, ctrl);
			return 0;
			break;
		case USB_REQ_DFU_ABORT:
			dfu_abort(dfu);
			value = 0;
			break;
		default:
			dfu->dfu_state = DFU_STATE_dfuERROR;
			value = -EINVAL;
			break;
		}
		break;
	case DFU_STATE_dfuERROR:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_CLRSTATUS:
			dfu_abort(dfu);
			/* no zlp? */
			value = 0;
			break;
		default:
			dfu->dfu_state = DFU_STATE_dfuERROR;
			value = -EINVAL;
			break;
		}
		break;
	case DFU_STATE_dfuDNLOAD_SYNC:
	case DFU_STATE_dfuDNBUSY:
	case DFU_STATE_dfuMANIFEST_SYNC:
	case DFU_STATE_dfuMANIFEST:
	case DFU_STATE_dfuMANIFEST_WAIT_RST:
		dfu->dfu_state = DFU_STATE_dfuERROR;
		value = -EINVAL;
		goto out;
		break;
	default:
		break;
	}
out:
	/* respond with data transfer or status phase? */
	if (value >= 0) {
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req);
		if (value < 0)
			ERROR(cdev, "dfu response on ttyGS%d, err %d\n",
					dfu->port_num, value);
	}

	return value;
}

static void dfu_disable(struct usb_function *f)
{
	struct f_dfu		*dfu = func_to_dfu(f);

	switch (dfu->dfu_state) {
	case DFU_STATE_appDETACH:
		dfu->dfu_state = DFU_STATE_dfuIDLE;
		break;
	case DFU_STATE_dfuMANIFEST_WAIT_RST:
		dfu->dfu_state = DFU_STATE_appIDLE;
		break;
	default:
		dfu->dfu_state = DFU_STATE_appDETACH;
		break;
	}

	dfu_cleanup(dfu);
}

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_DESCRIPTION_IDX		2

static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = NULL,
	[STRING_PRODUCT_IDX].s = NULL,
	[STRING_DESCRIPTION_IDX].s = "USB Device Firmware Upgrade",
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static int dfu_bind_config(struct usb_configuration *c)
{
	struct f_dfu *dfu;
	struct usb_function *func;
	int		status;
	int i;

	/* config description */
	status = usb_string_id(c->cdev);
	if (status < 0)
		return status;
	strings_dev[STRING_DESCRIPTION_IDX].id = status;

	status = usb_string_id(c->cdev);
	if (status < 0)
		return status;
	dfu_control_interface_desc.iInterface = status;

	/* allocate and initialize one new instance */
	dfu = xzalloc(sizeof *dfu);

	dfu->dfu_state = DFU_STATE_appIDLE;
	dfu->dfu_status = DFU_STATUS_OK;

	dfu_string_defs = xzalloc(sizeof(struct usb_string) * (dfu_num_alt + 2));
	dfu_string_defs[0].s = "Generic DFU";
	dfu_string_defs[0].id = status;
	for (i = 0; i < dfu_num_alt; i++) {
		dfu_string_defs[i + 1].s = dfu_devs[i].name;
		status = usb_string_id(c->cdev);
		if (status < 0)
			goto out;
		dfu_string_defs[i + 1].id = status;
	}
	dfu_string_defs[i + 1].s = NULL;
	dfu_string_table.strings = dfu_string_defs;

	func = &dfu->func;
	func->name = "DFU";
	func->strings = dfu_strings;
	/* descriptors are per-instance copies */
	func->bind = dfu_bind;
	func->unbind = dfu_unbind;
	func->set_alt = dfu_set_alt;
	func->setup = dfu_setup;
	func->disable = dfu_disable;

	dfu->dnreq = usb_ep_alloc_request(c->cdev->gadget->ep0);
	if (!dfu->dnreq)
		printf("usb_ep_alloc_request failed\n");
	dfu->dnreq->buf = dma_alloc(CONFIG_USBD_DFU_XFER_SIZE);
	dfu->dnreq->complete = dn_complete;
	dfu->dnreq->zero = 0;

	status = usb_add_function(c, func);
	if (status)
		goto out;

	return 0;
out:
	free(dfu);
	free(dfu_string_defs);

	return status;
}

static void dfu_unbind_config(struct usb_configuration *c)
{
	free(dfu_string_defs);
}

static struct usb_configuration dfu_config_driver = {
	.label			= "USB DFU",
	.bind			= dfu_bind_config,
	.unbind			= dfu_unbind_config,
	.bConfigurationValue	= 1,
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
};

static struct usb_device_descriptor dfu_dev_descriptor = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	.bcdUSB			= 0x0100,
	.bDeviceClass		= 0x00,
	.bDeviceSubClass	= 0x00,
	.bDeviceProtocol	= 0x00,
/*	.idVendor		= dynamic */
/*	.idProduct		= dynamic */
	.bcdDevice		= 0x0000,
	.bNumConfigurations	= 0x01,
};

static int dfu_driver_bind(struct usb_composite_dev *cdev)
{
	int status;

	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_MANUFACTURER_IDX].id = status;
	dfu_dev_descriptor.iManufacturer = status;

	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_PRODUCT_IDX].id = status;
	dfu_dev_descriptor.iProduct = status;

	/* config description */
	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_DESCRIPTION_IDX].id = status;
	dfu_config_driver.iConfiguration = status;

	status = usb_add_config(cdev, &dfu_config_driver);
	if (status < 0)
		goto fail;

	return 0;
fail:
	return status;
}

static struct usb_composite_driver dfu_driver = {
	.name		= "g_dfu",
	.dev		= &dfu_dev_descriptor,
	.strings	= dev_strings,
	.bind		= dfu_driver_bind,
};

int usb_dfu_register(struct usb_dfu_pdata *pdata)
{
	dfu_devs = pdata->alts;
	dfu_num_alt = pdata->num_alts;
	dfu_dev_descriptor.idVendor = pdata->idVendor;
	dfu_dev_descriptor.idProduct = pdata->idProduct;
	strings_dev[STRING_MANUFACTURER_IDX].s = pdata->manufacturer;
	strings_dev[STRING_PRODUCT_IDX].s = pdata->productname;

	usb_composite_register(&dfu_driver);

	while (1) {
		usb_gadget_poll();
		if (ctrlc() || dfudetach)
			goto out;
	}

out:
	dfudetach = 0;
	usb_composite_unregister(&dfu_driver);

	return 0;
}

