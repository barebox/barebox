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
#include <libfile.h>
#include <linux/err.h>
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
#include <ioctl.h>
#include <linux/mtd/mtd-abi.h>

#define USB_DT_DFU			0x21

struct usb_dfu_func_descriptor {
	u_int8_t		bLength;
	u_int8_t		bDescriptorType;
	u_int8_t		bmAttributes;
#define USB_DFU_CAN_DOWNLOAD	(1 << 0)
#define USB_DFU_CAN_UPLOAD	(1 << 1)
#define USB_DFU_MANIFEST_TOL	(1 << 2)
#define USB_DFU_WILL_DETACH	(1 << 3)
	u_int16_t		wDetachTimeOut;
	u_int16_t		wTransferSize;
	u_int16_t		bcdDFUVersion;
} __attribute__ ((packed));

#define USB_DT_DFU_SIZE			9

#define USB_TYPE_DFU		(USB_TYPE_CLASS|USB_RECIP_INTERFACE)

/* DFU class-specific requests (Section 3, DFU Rev 1.1) */
#define USB_REQ_DFU_DETACH	0x00
#define USB_REQ_DFU_DNLOAD	0x01
#define USB_REQ_DFU_UPLOAD	0x02
#define USB_REQ_DFU_GETSTATUS	0x03
#define USB_REQ_DFU_CLRSTATUS	0x04
#define USB_REQ_DFU_GETSTATE	0x05
#define USB_REQ_DFU_ABORT	0x06

struct dfu_status {
	u_int8_t bStatus;
	u_int8_t bwPollTimeout[3];
	u_int8_t bState;
	u_int8_t iString;
} __attribute__((packed));

#define DFU_STATUS_OK			0x00
#define DFU_STATUS_errTARGET		0x01
#define DFU_STATUS_errFILE		0x02
#define DFU_STATUS_errWRITE		0x03
#define DFU_STATUS_errERASE		0x04
#define DFU_STATUS_errCHECK_ERASED	0x05
#define DFU_STATUS_errPROG		0x06
#define DFU_STATUS_errVERIFY		0x07
#define DFU_STATUS_errADDRESS		0x08
#define DFU_STATUS_errNOTDONE		0x09
#define DFU_STATUS_errFIRMWARE		0x0a
#define DFU_STATUS_errVENDOR		0x0b
#define DFU_STATUS_errUSBR		0x0c
#define DFU_STATUS_errPOR		0x0d
#define DFU_STATUS_errUNKNOWN		0x0e
#define DFU_STATUS_errSTALLEDPKT	0x0f

enum dfu_state {
	DFU_STATE_appIDLE		= 0,
	DFU_STATE_appDETACH		= 1,
	DFU_STATE_dfuIDLE		= 2,
	DFU_STATE_dfuDNLOAD_SYNC	= 3,
	DFU_STATE_dfuDNBUSY		= 4,
	DFU_STATE_dfuDNLOAD_IDLE	= 5,
	DFU_STATE_dfuMANIFEST_SYNC	= 6,
	DFU_STATE_dfuMANIFEST		= 7,
	DFU_STATE_dfuMANIFEST_WAIT_RST	= 8,
	DFU_STATE_dfuUPLOAD_IDLE	= 9,
	DFU_STATE_dfuERROR		= 10,
};

#define USB_DT_DFU_SIZE			9
#define USB_DT_DFU			0x21

#define CONFIG_USBD_DFU_XFER_SIZE     4096
#define DFU_TEMPFILE "/dfu_temp"

struct file_list_entry *dfu_file_entry;
static int dfufd = -EINVAL;
static struct file_list *dfu_files;
static int dfudetach;
static struct mtd_info_user dfu_mtdinfo;
static loff_t dfu_written;
static loff_t dfu_erased;
static int prog_erase;

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

static void dn_complete(struct usb_ep *ep, struct usb_request *req);

static int
dfu_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_descriptor_header **header;
	struct usb_interface_descriptor *desc;
	struct file_list_entry *fentry;
	struct f_dfu *dfu = container_of(f, struct f_dfu, func);
	int i;
	int			status;
	struct usb_string	*us;

	if (!dfu_files) {
		const struct usb_function_instance *fi = f->fi;
		struct f_dfu_opts *opts = container_of(fi, struct f_dfu_opts, func_inst);

		dfu_files = opts->files;
	}

	dfu_string_defs = xzalloc(sizeof(struct usb_string) * (dfu_files->num_entries + 2));
	dfu_string_defs[0].s = "Generic DFU";
	i = 0;
	file_list_for_each_entry(dfu_files, fentry) {
		dfu_string_defs[i + 1].s = fentry->name;
		i++;
	}

	dfu_string_defs[i + 1].s = NULL;
	dfu_string_table.strings = dfu_string_defs;

	dfu->dfu_state = DFU_STATE_dfuIDLE;
	dfu->dfu_status = DFU_STATUS_OK;

	dfu->dnreq = usb_ep_alloc_request(c->cdev->gadget->ep0);
	if (!dfu->dnreq) {
		printf("usb_ep_alloc_request failed\n");
		status = -ENOMEM;
		goto out;
	}
	dfu->dnreq->buf = dma_alloc(CONFIG_USBD_DFU_XFER_SIZE);
	dfu->dnreq->complete = dn_complete;
	dfu->dnreq->zero = 0;

	us = usb_gstrings_attach(cdev, dfu_strings, dfu_files->num_entries + 1);
	if (IS_ERR(us)) {
		status = PTR_ERR(us);
		goto out;
	}

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto out;

	header = xzalloc(sizeof(void *) * (dfu_files->num_entries + 2));
	desc = xzalloc(sizeof(struct usb_interface_descriptor) * dfu_files->num_entries);
	for (i = 0; i < dfu_files->num_entries; i++) {
		desc[i].bLength =		USB_DT_INTERFACE_SIZE;
		desc[i].bDescriptorType =	USB_DT_INTERFACE;
		desc[i].bNumEndpoints	=	0;
		desc[i].bInterfaceClass =	0xfe;
		desc[i].bInterfaceSubClass =	1;
		desc[i].bInterfaceProtocol =	2;
		desc[i].bAlternateSetting = i;
		desc[i].iInterface = us[i + 1].id;
		header[i] = (struct usb_descriptor_header *)&desc[i];
	}
	header[i] = (struct usb_descriptor_header *) &usb_dfu_func;
	header[i + 1] = NULL;

	status = usb_assign_descriptors(f, header, header, NULL);

	free(desc);
	free(header);

	if (status)
		goto out;

	i = 0;
	file_list_for_each_entry(dfu_files, fentry) {
		printf("dfu: register alt%d(%s) with device %s\n",
				i, fentry->name, fentry->filename);
		i++;
	}

	return 0;
out:
	free(dfu_string_defs);

	if (status)
		ERROR(cdev, "%s/%p: can't bind, err %d\n", f->name, f, status);

	return status;
}

static void
dfu_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_dfu		*dfu = func_to_dfu(f);

	dfu_files = NULL;
	dfu_file_entry = NULL;
	dfudetach = 0;

	usb_free_all_descriptors(f);

	dma_free(dfu->dnreq->buf);
	usb_ep_free_request(c->cdev->gadget->ep0, dfu->dnreq);
}

static int dfu_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct file_list_entry	*fentry;
	int i = 0;

	file_list_for_each_entry(dfu_files, fentry) {
		if (i == alt) {
			dfu_file_entry = fentry;
			return 0;
		}

		i++;
	}

	return -EINVAL;
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

	memset(&dfu_mtdinfo, 0, sizeof(dfu_mtdinfo));
	dfu_written = 0;
	dfu_erased = 0;
	prog_erase = 0;

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
	loff_t size;
	int ret;

	if (prog_erase && (dfu_written + req->length) > dfu_erased) {
		size = roundup(req->length, dfu_mtdinfo.erasesize);
		ret = erase(dfufd, size, dfu_erased);
		dfu_erased += size;
		if (ret && ret != -ENOSYS) {
			perror("erase");
			dfu->dfu_status = DFU_STATUS_errERASE;
			dfu_cleanup(dfu);
			return;
		}
	}

	dfu_written += req->length;
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

	if (w_length == 0) {
		if (dfu_file_entry->flags & FILE_LIST_FLAG_SAFE) {
			dfu->dfu_state = DFU_STATE_dfuMANIFEST;
		} else {
			dfu->dfu_state = DFU_STATE_dfuIDLE;
			dfu_cleanup(dfu);
		}
		return 0;
	}

	dfu->dnreq->length = w_length;
	dfu->dnreq->context = dfu;
	usb_ep_queue(cdev->gadget->ep0, dfu->dnreq);

	return 0;
}

static int handle_manifest(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	int ret;

	dfu->dfu_state = DFU_STATE_dfuIDLE;

	if (dfu_file_entry->flags & FILE_LIST_FLAG_SAFE) {
		int fd;
		unsigned flags = O_WRONLY;

		if (dfu_file_entry->flags & FILE_LIST_FLAG_CREATE)
			flags |= O_CREAT | O_TRUNC;

		fd = open(dfu_file_entry->filename, flags);
		if (fd < 0) {
			perror("open");
			dfu->dfu_status = DFU_STATUS_errERASE;
			ret = -EINVAL;
			goto out;
		}

		ret = erase(fd, ERASE_SIZE_ALL, 0);
		close(fd);
		if (ret && ret != -ENOSYS) {
			dfu->dfu_status = DFU_STATUS_errERASE;
			perror("erase");
			goto out;
		}

		ret = copy_file(DFU_TEMPFILE, dfu_file_entry->filename, 0);
		if (ret) {
			printf("copy file failed\n");
			ret = -EINVAL;
			goto out;
		}
	}

	return 0;

out:
	dfu->dfu_status = DFU_STATUS_errWRITE;
	dfu->dfu_state = DFU_STATE_dfuERROR;
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

	switch (dfu->dfu_state) {
	case DFU_STATE_dfuIDLE:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_GETSTATUS:
			value = dfu_status(f, ctrl);
			value = min(value, w_length);
			break;
		case USB_REQ_DFU_GETSTATE:
			*(u8 *)req->buf = dfu->dfu_state;
			value = sizeof(u8);
			break;
		case USB_REQ_DFU_DNLOAD:
			if (w_length == 0) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				value = -EINVAL;
				goto out;
			}
			debug("dfu: starting download to %s\n", dfu_file_entry->filename);
			if (dfu_file_entry->flags & FILE_LIST_FLAG_SAFE) {
				dfufd = open(DFU_TEMPFILE, O_WRONLY | O_CREAT);
			} else {
				unsigned flags = O_WRONLY;

				if (dfu_file_entry->flags & FILE_LIST_FLAG_CREATE)
					flags |= O_CREAT | O_TRUNC;

				dfufd = open(dfu_file_entry->filename, flags);
			}

			if (dfufd < 0) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				perror("open");
				goto out;
			}

			if (!(dfu_file_entry->flags & FILE_LIST_FLAG_SAFE)) {
				ret = ioctl(dfufd, MEMGETINFO, &dfu_mtdinfo);
				if (!ret) /* file is on a mtd device */
					prog_erase = 1;
			}

			value = handle_dnload(f, ctrl);
			dfu->dfu_state = DFU_STATE_dfuDNLOAD_IDLE;
			return 0;
		case USB_REQ_DFU_UPLOAD:
			dfu->dfu_state = DFU_STATE_dfuUPLOAD_IDLE;
			debug("dfu: starting upload from %s\n", dfu_file_entry->filename);
			if (!(dfu_file_entry->flags & FILE_LIST_FLAG_READBACK)) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				goto out;
			}
			dfufd = open(dfu_file_entry->filename, O_RDONLY);
			if (dfufd < 0) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				perror("open");
				goto out;
			}
			handle_upload(f, ctrl);
			return 0;
		case USB_REQ_DFU_ABORT:
			dfu->dfu_status = DFU_STATUS_OK;
			value = 0;
			break;
		case USB_REQ_DFU_DETACH:
			value = 0;
			dfudetach = 1;
			break;
		default:
			dfu->dfu_state = DFU_STATE_dfuERROR;
			value = -EINVAL;
			break;
		}
		break;
	case DFU_STATE_dfuDNLOAD_IDLE:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_GETSTATUS:
			value = dfu_status(f, ctrl);
			value = min(value, w_length);
			break;
		case USB_REQ_DFU_GETSTATE:
			*(u8 *)req->buf = dfu->dfu_state;
			value = sizeof(u8);
			break;
		case USB_REQ_DFU_DNLOAD:
			value = handle_dnload(f, ctrl);
			if (dfu->dfu_state == DFU_STATE_dfuDNLOAD_IDLE) {
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
		case USB_REQ_DFU_GETSTATUS:
			value = dfu_status(f, ctrl);
			value = min(value, w_length);
			break;
		case USB_REQ_DFU_GETSTATE:
			*(u8 *)req->buf = dfu->dfu_state;
			value = sizeof(u8);
			break;
		case USB_REQ_DFU_UPLOAD:
			handle_upload(f, ctrl);
			return 0;
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
		case USB_REQ_DFU_GETSTATUS:
			value = dfu_status(f, ctrl);
			value = min(value, w_length);
			break;
		case USB_REQ_DFU_GETSTATE:
			*(u8 *)req->buf = dfu->dfu_state;
			value = sizeof(u8);
			break;
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
	case DFU_STATE_dfuMANIFEST_SYNC:
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_GETSTATUS:
			value = dfu_status(f, ctrl);
			if (dfu_file_entry->flags & FILE_LIST_FLAG_SAFE)
				dfu->dfu_state = DFU_STATE_dfuMANIFEST;
			else
				dfu->dfu_state = DFU_STATE_dfuIDLE;
			value = min(value, w_length);
			break;
		case USB_REQ_DFU_GETSTATE:
			*(u8 *)req->buf = dfu->dfu_state;
			value = sizeof(u8);
			break;
		default:
			dfu->dfu_state = DFU_STATE_dfuERROR;
			value = -EINVAL;
			break;
		}
		break;
	case DFU_STATE_dfuMANIFEST:
		value = handle_manifest(f, ctrl);
		if (dfu->dfu_state != DFU_STATE_dfuIDLE) {
			return 0;
		}
		switch (ctrl->bRequest) {
		case USB_REQ_DFU_GETSTATUS:
			value = dfu_status(f, ctrl);
			value = min(value, w_length);
			break;
		case USB_REQ_DFU_GETSTATE:
			*(u8 *)req->buf = dfu->dfu_state;
			value = sizeof(u8);
			break;
		default:
			dfu->dfu_state = DFU_STATE_dfuERROR;
			value = -EINVAL;
			break;
		}
		break;
	case DFU_STATE_dfuDNLOAD_SYNC:
	case DFU_STATE_dfuDNBUSY:
		dfu->dfu_state = DFU_STATE_dfuERROR;
		value = -EINVAL;
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

	dfu->dfu_state = DFU_STATE_dfuIDLE;

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

static void dfu_unbind_config(struct usb_configuration *c)
{
	free(dfu_string_defs);
}

static struct usb_configuration dfu_config_driver = {
	.label			= "USB DFU",
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

static struct usb_function_instance *fi_dfu;
static struct usb_function *f_dfu;

static int dfu_driver_bind(struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;
	int status;

	if (gadget->vendor_id && gadget->product_id) {
		dfu_dev_descriptor.idVendor = cpu_to_le16(gadget->vendor_id);
		dfu_dev_descriptor.idProduct = cpu_to_le16(gadget->product_id);
	} else {
		dfu_dev_descriptor.idVendor = cpu_to_le16(0x1d50); /* Openmoko, Inc */
		dfu_dev_descriptor.idProduct = cpu_to_le16(0x60a2); /* barebox bootloader USB DFU Mode */
	}

	strings_dev[STRING_MANUFACTURER_IDX].s = gadget->manufacturer;
	strings_dev[STRING_PRODUCT_IDX].s = gadget->productname;

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

	status = usb_add_config_only(cdev, &dfu_config_driver);
	if (status < 0)
		goto fail;

	fi_dfu = usb_get_function_instance("dfu");
	if (IS_ERR(fi_dfu)) {
		status = PTR_ERR(fi_dfu);
		goto fail;
	}

	f_dfu = usb_get_function(fi_dfu);
	if (IS_ERR(f_dfu)) {
		status = PTR_ERR(f_dfu);
		goto fail;
	}

	status = usb_add_function(&dfu_config_driver, f_dfu);
	if (status)
		goto fail;

	return 0;
fail:
	return status;
}

static int dfu_driver_unbind(struct usb_composite_dev *cdev)
{
	usb_put_function(f_dfu);
	usb_put_function_instance(fi_dfu);

	return 0;
}

static struct usb_composite_driver dfu_driver = {
	.name		= "g_dfu",
	.dev		= &dfu_dev_descriptor,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= dfu_driver_bind,
	.unbind		= dfu_driver_unbind,
};

int usb_dfu_register(struct f_dfu_opts *opts)
{
	int ret;

	if (dfu_files)
		return -EBUSY;

	dfu_files = opts->files;

	ret = usb_composite_probe(&dfu_driver);
	if (ret)
		goto out;

	while (1) {
		ret = usb_gadget_poll();
		if (ret < 0)
			goto out1;

		if (dfudetach) {
			ret = 0;
			goto out1;
		}

		if (ctrlc()) {
			ret = -EINTR;
			goto out1;
		}
	}

out1:
	dfudetach = 0;
	usb_composite_unregister(&dfu_driver);
out:
	dfu_files = NULL;

	return ret;
}

static void dfu_free_func(struct usb_function *f)
{
	struct f_dfu *dfu = container_of(f, struct f_dfu, func);

	free(dfu);
}

static struct usb_function *dfu_alloc_func(struct usb_function_instance *fi)
{
	struct f_dfu *dfu;

	dfu = xzalloc(sizeof(*dfu));

	dfu->func.name = "dfu";
	dfu->func.strings = dfu_strings;
	/* descriptors are per-instance copies */
	dfu->func.bind = dfu_bind;
	dfu->func.set_alt = dfu_set_alt;
	dfu->func.setup = dfu_setup;
	dfu->func.disable = dfu_disable;
	dfu->func.unbind = dfu_unbind;
	dfu->func.free_func = dfu_free_func;

	return &dfu->func;
}

static void dfu_free_instance(struct usb_function_instance *fi)
{
	struct f_dfu_opts *opts;

	opts = container_of(fi, struct f_dfu_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *dfu_alloc_instance(void)
{
	struct f_dfu_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	opts->func_inst.free_func_inst = dfu_free_instance;

	return &opts->func_inst;
}

DECLARE_USB_FUNCTION_INIT(dfu, dfu_alloc_instance, dfu_alloc_func);
