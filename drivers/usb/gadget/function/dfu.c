// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) 2007 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * based on existing SAM7DFU code from OpenPCD:
 * (C) Copyright 2006 by Harald Welte <hwelte@hmw-consulting.de>
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
#define pr_fmt(fmt) "dfu: " fmt

#include <dma.h>
#include <asm/byteorder.h>
#include <linux/usb/composite.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/usb/gadget.h>
#include <linux/stat.h>
#include <libfile.h>
#include <linux/err.h>
#include <linux/usb/ch9.h>
#include <linux/usb/dfu.h>
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
#include <work.h>

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
	struct work_queue wq;
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
static void up_complete(struct usb_ep *ep, struct usb_request *req);
static void dfu_cleanup(struct f_dfu *dfu);

struct dfu_work {
	struct work_struct work;
	struct f_dfu *dfu;
	void (*task)(struct dfu_work *dw);
	size_t len;
	uint8_t *rbuf;
	uint8_t wbuf[CONFIG_USBD_DFU_XFER_SIZE];
};

static void dfu_do_work(struct work_struct *w)
{
	struct dfu_work *dw = container_of(w, struct dfu_work, work);
	struct f_dfu *dfu = dw->dfu;

	if (dfu->dfu_state != DFU_STATE_dfuERROR && dfu->dfu_status == DFU_STATUS_OK)
		dw->task(dw);
	else
		pr_debug("skip work\n");

	free(dw);
}

static void dfu_work_cancel(struct work_struct *w)
{
	struct dfu_work *dw = container_of(w, struct dfu_work, work);

	free(dw);
}

static void dfu_do_write(struct dfu_work *dw)
{
	struct f_dfu *dfu = dw->dfu;
	ssize_t size, wlen = dw->len;
	ssize_t ret;

	pr_debug("do write\n");

	if (prog_erase && (dfu_written + wlen) > dfu_erased) {
		size = roundup(wlen, dfu_mtdinfo.erasesize);
		ret = erase(dfufd, size, dfu_erased, ERASE_TO_WRITE);
		dfu_erased += size;
		if (ret && ret != -ENOSYS) {
			perror("erase");
			dfu->dfu_state = DFU_STATE_dfuERROR;
			dfu->dfu_status = DFU_STATUS_errERASE;
			return;
		}
	}

	dfu_written += wlen;
	ret = write(dfufd, dw->wbuf, wlen);
	if (ret < wlen) {
		perror("write");
		dfu->dfu_state = DFU_STATE_dfuERROR;
		dfu->dfu_status = DFU_STATUS_errWRITE;
	}
}

static void dfu_do_read(struct dfu_work *dw)
{
	struct f_dfu *dfu = dw->dfu;
	struct usb_composite_dev *cdev = dfu->func.config->cdev;
	ssize_t size, rlen = dw->len;

	pr_debug("do read\n");

	size = read(dfufd, dfu->dnreq->buf, rlen);
	dfu->dnreq->length = size;
	if (size < 0) {
		perror("read");
		dfu->dnreq->length = 0;
		dfu->dfu_state = DFU_STATE_dfuERROR;
		dfu->dfu_status = DFU_STATUS_errFILE;
	} else if (size < rlen) {
		/* this is the last chunk, go to IDLE and close file */
		dfu_cleanup(dfu);
	}

	dfu->dnreq->complete = up_complete;
	usb_ep_queue(cdev->gadget->ep0, dfu->dnreq);
}

static void dfu_do_open_dnload(struct dfu_work *dw)
{
	struct f_dfu *dfu = dw->dfu;
	int ret;

	pr_debug("do open dnload\n");

	if (dfu_file_entry->flags & FILE_LIST_FLAG_SAFE) {
		dfufd = open(DFU_TEMPFILE, O_WRONLY | O_CREAT);
	} else {
		unsigned flags = O_WRONLY;

		if (dfu_file_entry->flags & FILE_LIST_FLAG_CREATE)
			flags |= O_CREAT | O_TRUNC;

		dfufd = open(dfu_file_entry->filename, flags);
	}

	if (dfufd < 0) {
		perror("open");
		dfu->dfu_state = DFU_STATE_dfuERROR;
		dfu->dfu_status = DFU_STATUS_errFILE;
		return;
	}

	if (!(dfu_file_entry->flags & FILE_LIST_FLAG_SAFE)) {
		ret = ioctl(dfufd, MEMGETINFO, &dfu_mtdinfo);
		if (!ret) /* file is on a mtd device */
			prog_erase = 1;
	}
}

static void dfu_do_open_upload(struct dfu_work *dw)
{
	struct f_dfu *dfu = dw->dfu;

	pr_debug("do open upload\n");

	dfufd = open(dfu_file_entry->filename, O_RDONLY);
	if (dfufd < 0) {
		perror("open");
		dfu->dfu_state = DFU_STATE_dfuERROR;
		dfu->dfu_status = DFU_STATUS_errFILE;
	}
}

static void dfu_do_close(struct dfu_work *dw)
{
	struct stat s;

	pr_debug("do close\n");

	if (dfufd > 0) {
		close(dfufd);
		dfufd = -EINVAL;
	}

	if (!stat(DFU_TEMPFILE, &s))
		unlink(DFU_TEMPFILE);

	dw->dfu->dfu_state = DFU_STATE_dfuIDLE;
}

static void dfu_do_copy(struct dfu_work *dw)
{
	struct f_dfu *dfu = dw->dfu;
	unsigned flags = O_WRONLY;
	int ret, fd;

	pr_debug("do copy\n");

	if (dfu_file_entry->flags & FILE_LIST_FLAG_CREATE)
		flags |= O_CREAT | O_TRUNC;

	fd = open(dfu_file_entry->filename, flags);
	if (fd < 0) {
		perror("open");
		dfu->dfu_state = DFU_STATE_dfuERROR;
		dfu->dfu_status = DFU_STATUS_errERASE;
		return;
	}

	ret = erase(fd, ERASE_SIZE_ALL, 0, ERASE_TO_WRITE);
	close(fd);
	if (ret && ret != -ENOSYS) {
		perror("erase");
		dfu->dfu_state = DFU_STATE_dfuERROR;
		dfu->dfu_status = DFU_STATUS_errERASE;
		return;
	}

	ret = copy_file(DFU_TEMPFILE, dfu_file_entry->filename, 0);
	if (ret) {
		pr_err("copy file failed\n");
		dfu->dfu_state = DFU_STATE_dfuERROR;
		dfu->dfu_status = DFU_STATUS_errWRITE;
		return;
	}

	dfu->dfu_state = DFU_STATE_dfuIDLE;
}

static int
dfu_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_descriptor_header **header;
	struct usb_interface_descriptor *desc;
	struct file_list_entry *fentry;
	struct f_dfu *dfu = func_to_dfu(f);
	int i, n_entries;
	int			status;
	struct usb_string	*us;

	if (!dfu_files) {
		const struct usb_function_instance *fi = f->fi;
		struct f_dfu_opts *opts = container_of(fi, struct f_dfu_opts, func_inst);

		dfu_files = opts->files;
	}

	file_list_detect_all(dfu_files);

	n_entries = list_count_nodes(&dfu_files->list);

	dfu_string_defs = xzalloc(sizeof(struct usb_string) * (n_entries + 2));
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
		pr_err("usb_ep_alloc_request failed\n");
		status = -ENOMEM;
		goto out;
	}
	dfu->dnreq->buf = dma_alloc(CONFIG_USBD_DFU_XFER_SIZE);
	dfu->dnreq->complete = dn_complete;
	dfu->dnreq->zero = 0;

	us = usb_gstrings_attach(cdev, dfu_strings, n_entries + 1);
	if (IS_ERR(us)) {
		status = PTR_ERR(us);
		goto out;
	}

	dfu->wq.fn = dfu_do_work;
	dfu->wq.cancel = dfu_work_cancel;
	wq_register(&dfu->wq);

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto out;

	header = xzalloc(sizeof(void *) * (n_entries + 2));
	desc = xzalloc(sizeof(struct usb_interface_descriptor) * n_entries);
	for (i = 0; i < n_entries; i++) {
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

	status = usb_assign_descriptors(f, header, header, header, header);

	free(desc);
	free(header);

	if (status)
		goto out;

	i = 0;
	file_list_for_each_entry(dfu_files, fentry) {
		pr_info("register alt%d(%s) with device %s\n", i, fentry->name, fentry->filename);
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

	wq_unregister(&dfu->wq);

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

static int dfu_get_alt(struct usb_function *f, unsigned intf)
{
	struct file_list_entry  *fentry;
	int i = 0;

	file_list_for_each_entry(dfu_files, fentry) {
		if (fentry == dfu_file_entry)
			return i;
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
	struct dfu_work *dw;

	pr_debug("dfu cleanup\n");

	memset(&dfu_mtdinfo, 0, sizeof(dfu_mtdinfo));
	dfu_written = 0;
	dfu_erased = 0;
	prog_erase = 0;

	dfu->dfu_state = DFU_STATE_dfuIDLE;
	dfu->dfu_status = DFU_STATUS_OK;

	dw = xzalloc(sizeof(*dw));
	dw->dfu = dfu;
	dw->task = dfu_do_close;
	wq_queue_work(&dfu->wq, &dw->work);
}

static void dn_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_dfu		*dfu = req->context;
	struct dfu_work *dw;

	dw = xzalloc(sizeof(*dw));
	dw->dfu = dfu;
	dw->task = dfu_do_write;
	dw->len = min_t(unsigned int, req->length, CONFIG_USBD_DFU_XFER_SIZE);
	memcpy(dw->wbuf, req->buf, dw->len);
	wq_queue_work(&dfu->wq, &dw->work);
}

static int handle_manifest(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	struct dfu_work *dw;

	if (dfu_file_entry->flags & FILE_LIST_FLAG_SAFE) {
		dw = xzalloc(sizeof(*dw));
		dw->dfu = dfu;
		dw->task = dfu_do_copy;
		wq_queue_work(&dfu->wq, &dw->work);
	}

	dw = xzalloc(sizeof(*dw));
	dw->dfu = dfu;
	dw->task = dfu_do_close;
	wq_queue_work(&dfu->wq, &dw->work);

	return 0;
}

static int handle_dnload(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	u16			w_length = le16_to_cpu(ctrl->wLength);

	if (w_length == 0) {
		handle_manifest(f, ctrl);
		dfu->dfu_state = DFU_STATE_dfuMANIFEST;
		return 0;
	}

	dfu->dnreq->length = w_length;
	dfu->dnreq->context = dfu;
	usb_ep_queue(cdev->gadget->ep0, dfu->dnreq);

	return 0;
}

static void up_complete(struct usb_ep *ep, struct usb_request *req)
{
}

static int handle_upload(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfu		*dfu = func_to_dfu(f);
	struct dfu_work *dw;
	u16			w_length = le16_to_cpu(ctrl->wLength);

	dw = xzalloc(sizeof(*dw));
	dw->dfu = dfu;
	dw->task = dfu_do_read;
	dw->len = w_length;
	dw->rbuf = dfu->dnreq->buf;
	wq_queue_work(&dfu->wq, &dw->work);

	return 0;
}

static void dfu_abort(struct f_dfu *dfu)
{
	wq_cancel_work(&dfu->wq);

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
	struct dfu_work *dw;

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
			pr_debug("starting download to %s\n", dfu_file_entry->filename);
			dw = xzalloc(sizeof(*dw));
			dw->dfu = dfu;
			dw->task = dfu_do_open_dnload;
			wq_queue_work(&dfu->wq, &dw->work);

			value = handle_dnload(f, ctrl);
			dfu->dfu_state = DFU_STATE_dfuDNLOAD_IDLE;
			return 0;
		case USB_REQ_DFU_UPLOAD:
			dfu->dfu_state = DFU_STATE_dfuUPLOAD_IDLE;
			pr_debug("starting upload from %s\n", dfu_file_entry->filename);
			if (!(dfu_file_entry->flags & FILE_LIST_FLAG_READBACK)) {
				dfu->dfu_state = DFU_STATE_dfuERROR;
				goto out;
			}

			dw = xzalloc(sizeof(*dw));
			dw->dfu = dfu;
			dw->task = dfu_do_open_upload;
			wq_queue_work(&dfu->wq, &dw->work);

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
		wq_cancel_work(&dfu->wq);
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
			dfu->dfu_state = DFU_STATE_dfuMANIFEST;
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
	case DFU_STATE_dfuMANIFEST:
		dfu->dfu_state = DFU_STATE_dfuMANIFEST_SYNC;
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
		value = composite_queue_setup_request(cdev);
		if (value < 0)
			ERROR(cdev, "dfu response on ttyGS%d, err %d\n",
					dfu->port_num, value);
	}

	return value;
}

static void dfu_disable(struct usb_function *f)
{
	struct f_dfu		*dfu = func_to_dfu(f);

	dfu_abort(dfu);
}

int usb_dfu_detached(void)
{
	return dfudetach;
}

static void dfu_free_func(struct usb_function *f)
{
	struct f_dfu *dfu = func_to_dfu(f);

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
	dfu->func.get_alt = dfu_get_alt;
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
