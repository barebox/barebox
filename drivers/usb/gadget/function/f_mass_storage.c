// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * f_mass_storage.c -- Mass Storage USB Composite Function
 *
 * Copyright (C) 2003-2008 Alan Stern
 * Copyright (C) 2009 Samsung Electronics
 *                    Author: Michal Nazarewicz <m.nazarewicz@samsung.com>
 * All rights reserved.
 */

/*
 * The Mass Storage Function acts as a USB Mass Storage device,
 * appearing to the host as a disk drive or as a CD-ROM drive.  In
 * addition to providing an example of a genuinely useful composite
 * function for a USB device, it also illustrates a technique of
 * double-buffering for increased throughput.
 *
 * Function supports multiple logical units (LUNs).  Backing storage
 * for each LUN is provided by a regular file or a block device.
 * Access for each LUN can be limited to read-only.  Moreover, the
 * function can indicate that LUN is removable and/or CD-ROM.  (The
 * later implies read-only access.)
 *
 * MSF is configured by specifying a fsg_config structure.  It has the
 * following fields:
 *
 *	nluns		Number of LUNs function have (anywhere from 1
 *				to FSG_MAX_LUNS which is 8).
 *	luns		An array of LUN configuration values.  This
 *				should be filled for each LUN that
 *				function will include (ie. for "nluns"
 *				LUNs).  Each element of the array has
 *				the following fields:
 *	->filename	The path to the backing file for the LUN.
 *				Required if LUN is not marked as
 *				removable.
 *	->ro		Flag specifying access to the LUN shall be
 *				read-only.  This is implied if CD-ROM
 *				emulation is enabled as well as when
 *				it was impossible to open "filename"
 *				in R/W mode.
 *	->removable	Flag specifying that LUN shall be indicated as
 *				being removable.
 *	->cdrom		Flag specifying that LUN shall be reported as
 *				being a CD-ROM.
 *
 *	vendor_name
 *	product_name
 *	release		Information used as a reply to INQUIRY
 *				request.  To use default set to NULL,
 *				NULL, 0xffff respectively.  The first
 *				field should be 8 and the second 16
 *				characters or less.
 *
 *	can_stall	Set to permit function to halt bulk endpoints.
 *				Disabled on some USB devices known not
 *				to work correctly.  You should set it
 *				to true.
 *
 * If "removable" is not set for a LUN then a backing file must be
 * specified.  If it is set, then NULL filename means the LUN's medium
 * is not loaded (an empty string as "filename" in the fsg_config
 * structure causes error).  The CD-ROM emulation includes a single
 * data track and no audio tracks; hence there need be only one
 * backing file per LUN.  Note also that the CD-ROM block length is
 * set to 512 rather than the more common value 2048.
 *
 *
 * MSF includes support for module parameters.  If gadget using it
 * decides to use it, the following module parameters will be
 * available:
 *
 *	file=filename[,filename...]
 *			Names of the files or block devices used for
 *				backing storage.
 *	ro=b[,b...]	Default false, boolean for read-only access.
 *	removable=b[,b...]
 *			Default true, boolean for removable media.
 *	cdrom=b[,b...]	Default false, boolean for whether to emulate
 *				a CD-ROM drive.
 *	luns=N		Default N = number of filenames, number of
 *				LUNs to support.
 *	stall		Default determined according to the type of
 *				USB device controller (usually true),
 *				boolean to permit the driver to halt
 *				bulk endpoints.
 *
 * The module parameters may be prefixed with some string.  You need
 * to consult gadget's documentation or source to verify whether it is
 * using those module parameters and if it does what are the prefixes
 * (look for FSG_MODULE_PARAMETERS() macro usage, what's inside it is
 * the prefix).
 *
 *
 * Requirements are modest; only a bulk-in and a bulk-out endpoint are
 * needed.  The memory requirement amounts to two 16K buffers, size
 * configurable by a parameter.  Support is included for both
 * full-speed and high-speed operation.
 *
 * Note that the driver is slightly non-portable in that it assumes a
 * single memory/DMA buffer will be useable for bulk-in, bulk-out, and
 * interrupt-in endpoints.  With most device controllers this isn't an
 * issue, but there may be some with hardware restrictions that prevent
 * a buffer from being used by more than one endpoint.
 *
 * When a LUN receive an "eject" SCSI request (Start/Stop Unit),
 * if the LUN is removable, the backing file is released to simulate
 * ejection.
 *
 *
 * This function is heavily based on "File-backed Storage Gadget" by
 * Alan Stern which in turn is heavily based on "Gadget Zero" by David
 * Brownell.  The driver's SCSI command interface was based on the
 * "Information technology - Small Computer System Interface - 2"
 * document from X3T9.2 Project 375D, Revision 10L, 7-SEP-93,
 * available at <http://www.t10.org/ftp/t10/drafts/s2/s2-r10l.pdf>.
 * The single exception is opcode 0x23 (READ FORMAT CAPACITIES), which
 * was based on the "Universal Serial Bus Mass Storage Class UFI
 * Command Specification" document, Revision 1.0, December 14, 1998,
 * available at
 * <http://www.usb.org/developers/devclass_docs/usbmass-ufi10.pdf>.
 */

/*
 *				Driver Design
 *
 * The MSF is fairly straightforward.  There is a main kernel
 * thread that handles most of the work.  Interrupt routines field
 * callbacks from the controller driver: bulk- and interrupt-request
 * completion notifications, endpoint-0 events, and disconnect events.
 * Completion events are passed to the main thread by wakeup calls.  Many
 * ep0 requests are handled at interrupt time, but SetInterface,
 * SetConfiguration, and device reset requests are forwarded to the
 * thread in the form of "exceptions" using SIGUSR1 signals (since they
 * should interrupt any ongoing file I/O operations).
 *
 * The thread's main routine implements the standard command/data/status
 * parts of a SCSI interaction.  It and its subroutines are full of tests
 * for pending signals/exceptions -- all this polling is necessary since
 * the kernel has no setjmp/longjmp equivalents.  (Maybe this is an
 * indication that the driver really wants to be running in userspace.)
 * An important point is that so long as the thread is alive it keeps an
 * open reference to the backing file.  This will prevent unmounting
 * the backing file's underlying filesystem and could cause problems
 * during system shutdown, for example.  To prevent such problems, the
 * thread catches INT, TERM, and KILL signals and converts them into
 * an EXIT exception.
 *
 * In normal operation the main thread is started during the gadget's
 * fsg_bind() callback and stopped during fsg_unbind().  But it can
 * also exit when it receives a signal, and there's no point leaving
 * the gadget running when the thread is dead.  At of this moment, MSF
 * provides no way to deregister the gadget when thread dies -- maybe
 * a callback functions is needed.
 *
 * To provide maximum throughput, the driver uses a circular pipeline of
 * buffer heads (struct fsg_buffhd).  In principle the pipeline can be
 * arbitrarily long; in practice the benefits don't justify having more
 * than 2 stages (i.e., double buffering).  But it helps to think of the
 * pipeline as being a long one.  Each buffer head contains a bulk-in and
 * a bulk-out request pointer (since the buffer can be used for both
 * output and input -- directions always are given from the host's
 * point of view) as well as a pointer to the buffer and various state
 * variables.
 *
 * Use of the pipeline follows a simple protocol.  There is a variable
 * (fsg->next_buffhd_to_fill) that points to the next buffer head to use.
 * At any time that buffer head may still be in use from an earlier
 * request, so each buffer head has a state variable indicating whether
 * it is EMPTY, FULL, or BUSY.  Typical use involves waiting for the
 * buffer head to be EMPTY, filling the buffer either by file I/O or by
 * USB I/O (during which the buffer head is BUSY), and marking the buffer
 * head FULL when the I/O is complete.  Then the buffer will be emptied
 * (again possibly by USB I/O, during which it is marked BUSY) and
 * finally marked EMPTY again (possibly by a completion routine).
 *
 * A module parameter tells the driver to avoid stalling the bulk
 * endpoints wherever the transport specification allows.  This is
 * necessary for some UDCs like the SuperH, which cannot reliably clear a
 * halt on a bulk endpoint.  However, under certain circumstances the
 * Bulk-only specification requires a stall.  In such cases the driver
 * will halt the endpoint and set a flag indicating that it should clear
 * the halt in software during the next device reset.  Hopefully this
 * will permit everything to work correctly.  Furthermore, although the
 * specification allows the bulk-out endpoint to halt when the host sends
 * too much data, implementing this would cause an unavoidable race.
 * The driver will always use the "no-stall" approach for OUT transfers.
 *
 * One subtle point concerns sending status-stage responses for ep0
 * requests.  Some of these requests, such as device reset, can involve
 * interrupting an ongoing file I/O operation, which might take an
 * arbitrarily long time.  During that delay the host might give up on
 * the original ep0 request and issue a new one.  When that happens the
 * driver should not notify the host about completion of the original
 * request, as the host will no longer be waiting for it.  So the driver
 * assigns to each ep0 request a unique tag, and it keeps track of the
 * tag value of the request associated with a long-running exception
 * (device-reset, interface-change, or configuration-change).  When the
 * exception handler is finished, the status-stage response is submitted
 * only if the current ep0 request tag is equal to the exception request
 * tag.  Thus only the most recently received ep0 request will get a
 * status-stage response.
 *
 * Warning: This driver source file is too long.  It ought to be split up
 * into a header file plus about 3 separate .c files, to handle the details
 * of the Gadget, USB Mass Storage, and SCSI protocols.
 */

/* #define VERBOSE_DEBUG */
/* #define DUMP_MSGS */

#define pr_fmt(fmt) "f_ums: " fmt

#include <common.h>
#include <unistd.h>
#include <linux/stat.h>
#include <linux/wait.h>
#include <fcntl.h>
#include <file-list.h>
#include <dma.h>
#include <linux/bug.h>
#include <linux/rwsem.h>
#include <linux/pagemap.h>
#include <disks.h>
#include <scsi.h>

#include <linux/err.h>
#include <linux/usb/mass_storage.h>

#include <asm/unaligned.h>
#include <linux/bitops.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/bitmap.h>
#include <linux/completion.h>
#include <bthread.h>
#include <sched.h>


/*------------------------------------------------------------------------*/

#define FSG_DRIVER_DESC	"ums"
#define UMS_NAME_LEN 16

#define FSG_DRIVER_VERSION	"2012/06/5"

static const char fsg_string_interface[] = "Mass Storage";

#include "storage_common.h"

/* Static strings, in UTF-8 (for simplicity we use only ASCII characters) */
struct usb_string		fsg_strings[] = {
	{FSG_STRING_INTERFACE,		fsg_string_interface},
	{}
};

static struct usb_gadget_strings	fsg_stringtab = {
	.language	= 0x0409,		/* en-us */
	.strings	= fsg_strings,
};

/*-------------------------------------------------------------------------*/

struct bthread *thread_task;

struct fsg_dev;

static struct file_list *ums_files;

/* Data shared by all the FSG instances. */
struct fsg_common {
	struct usb_gadget	*gadget;
	struct fsg_dev		*fsg, *new_fsg;

	struct usb_ep		*ep0;		/* Copy of gadget->ep0 */
	struct usb_request	*ep0req;	/* Copy of cdev->req */
	unsigned int		ep0_req_tag;

	struct fsg_buffhd	*next_buffhd_to_fill;
	struct fsg_buffhd	*next_buffhd_to_drain;
	struct fsg_buffhd	buffhds[FSG_NUM_BUFFERS];

	struct f_ums_opts	*opts;

	int			cmnd_size;
	u8			cmnd[MAX_COMMAND_SIZE];

	unsigned int		nluns;
	unsigned int		lun;
	struct fsg_lun          luns[FSG_MAX_LUNS];

	unsigned int		bulk_out_maxpacket;
	enum fsg_state		state;		/* For exception handling */
	unsigned int		exception_req_tag;

	enum data_direction	data_dir;
	u32			data_size;
	u32			data_size_from_cmnd;
	u32			tag;
	u32			residue;
	u32			usb_amount_left;

	unsigned int		can_stall:1;
	unsigned int		phase_error:1;
	unsigned int		short_packet_received:1;
	unsigned int		bad_lun_okay:1;
	unsigned int		running:1;

	struct completion	thread_wakeup_needed;

	/* Callback functions. */
	const struct fsg_operations	*ops;
	/* Gadget's private data. */
	void			*private_data;

	const char *vendor_name;		/*  8 characters or less */
	const char *product_name;		/* 16 characters or less */
	u16 release;

	/* Vendor (8 chars), product (16 chars), release (4
	 * hexadecimal digits) and NUL byte */
	char inquiry_string[8 + 16 + 4 + 1];
};

static struct f_ums_opts *f_ums_opts_get(struct f_ums_opts *opts)
{
	opts->refcnt++;
	return opts;
}

static void f_ums_opts_put(struct f_ums_opts *opts)
{
	if (--opts->refcnt == 0) {
		kfree(opts->common);
		kfree(opts);
	}
}

struct fsg_config {
	unsigned nluns;
	struct fsg_lun_config {
		const char *filename;
		char ro;
		char removable;
		char cdrom;
		char nofua;
	} luns[FSG_MAX_LUNS];

	/* Callback functions. */
	const struct fsg_operations     *ops;
	/* Gadget's private data. */
	void			*private_data;

	const char *vendor_name;		/*  8 characters or less */
	const char *product_name;		/* 16 characters or less */

	char			can_stall;
};

struct fsg_dev {
	struct usb_function	function;
	struct usb_gadget	*gadget;	/* Copy of cdev->gadget */
	struct fsg_common	*common;

	int			refcnt;

	u16			interface_number;

	unsigned int		bulk_in_enabled:1;
	unsigned int		bulk_out_enabled:1;

	unsigned long		atomic_bitflags;
#define IGNORE_BULK_OUT		0

	struct usb_ep		*bulk_in;
	struct usb_ep		*bulk_out;
};

static struct fsg_dev *fsg_dev_get(struct fsg_dev *fsg)
{
	fsg->refcnt++;
	return fsg;
}

static void fsg_dev_put(struct fsg_dev *fsg)
{
	if (--fsg->refcnt == 0)
		kfree(fsg);
}

static inline int __fsg_is_set(struct fsg_common *common,
			       const char *func, unsigned line)
{
	if (common->fsg)
		return 1;
	ERROR(common, "common->fsg is NULL in %s at %u\n", func, line);
	WARN_ON(1);

	return 0;
}

#define fsg_is_set(common) likely(__fsg_is_set(common, __func__, __LINE__))


static inline struct fsg_dev *fsg_from_func(struct usb_function *f)
{
	return container_of(f, struct fsg_dev, function);
}

static inline struct f_ums_opts *
fsg_opts_from_func_inst(const struct usb_function_instance *fi)
{
	return container_of(fi, struct f_ums_opts, func_inst);
}

typedef void (*fsg_routine_t)(struct fsg_dev *);

static int exception_in_progress(struct fsg_common *common)
{
	return common->state > FSG_STATE_IDLE;
}

/* Make bulk-out requests be divisible by the maxpacket size */
static void set_bulk_out_req_length(struct fsg_common *common,
		struct fsg_buffhd *bh, unsigned int length)
{
	unsigned int	rem;

	bh->bulk_out_intended_length = length;
	rem = length % common->bulk_out_maxpacket;
	if (rem > 0)
		length += common->bulk_out_maxpacket - rem;
	bh->outreq->length = length;
}

/*-------------------------------------------------------------------------*/

static struct f_ums_opts ums[14]; // FIXME
static int ums_count;

static int fsg_set_halt(struct fsg_dev *fsg, struct usb_ep *ep)
{
	const char	*name;

	if (ep == fsg->bulk_in)
		name = "bulk-in";
	else if (ep == fsg->bulk_out)
		name = "bulk-out";
	else
		name = ep->name;
	DBG(fsg, "%s set halt\n", name);
	return usb_ep_set_halt(ep);
}

/*-------------------------------------------------------------------------*/

/* These routines may be called in process context or in_irq */

/* Caller must hold fsg->lock */
static void wakeup_thread(struct fsg_common *common)
{
	complete(&common->thread_wakeup_needed);
}

static void report_exception(const char *prefix, enum fsg_state state)
{
	const char *msg = "<unknown>";
	switch (state) {
	/* This one isn't used anywhere */
	case FSG_STATE_COMMAND_PHASE:
		msg = "Command Phase";
		break;
	case FSG_STATE_DATA_PHASE:
		msg = "Data Phase";
		break;
	case FSG_STATE_STATUS_PHASE:
		msg = "Status Phase";
		break;

	case FSG_STATE_IDLE:
		msg = "Idle";
		break;
	case FSG_STATE_ABORT_BULK_OUT:
		msg = "abort bulk out";
		break;
	case FSG_STATE_RESET:
		msg = "reset";
		break;
	case FSG_STATE_INTERFACE_CHANGE:
		msg = "interface change";
		break;
	case FSG_STATE_CONFIG_CHANGE:
		msg = "config change";
		break;
	case FSG_STATE_DISCONNECT:
		msg = "disconnect";
		break;
	case FSG_STATE_EXIT:
		msg = "exit";
		break;
	case FSG_STATE_TERMINATED:
		msg = "terminated";
		break;
	}

	pr_debug("%s: %s\n", prefix, msg);
}

static void raise_exception(struct fsg_common *common, enum fsg_state new_state)
{
	/* Do nothing if a higher-priority exception is already in progress.
	 * If a lower-or-equal priority exception is in progress, preempt it
	 * and notify the main thread by sending it a signal. */
	if (common->state <= new_state) {
		report_exception("raising", new_state);
		common->exception_req_tag = common->ep0_req_tag;
		common->state = new_state;
		wakeup_thread(common);
	}
}

/*-------------------------------------------------------------------------*/

static int ep0_queue(struct fsg_common *common)
{
	int	rc;

	rc = usb_ep_queue(common->ep0, common->ep0req);
	common->ep0->driver_data = common;
	if (rc != 0 && rc != -ESHUTDOWN) {
		/* We can't do much more than wait for a reset */
		WARNING(common, "error in submission: %s --> %d\n",
			common->ep0->name, rc);
	}
	return rc;
}

/*-------------------------------------------------------------------------*/

/* Bulk and interrupt endpoint completion handlers.
 * These always run in_irq. */

static void bulk_in_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct fsg_common	*common = ep->driver_data;
	struct fsg_buffhd	*bh = req->context;

	if (req->status || req->actual != req->length)
		DBG(common, "%s --> %d, %u/%u\n", __func__,
				req->status, req->actual, req->length);
	if (req->status == -ECONNRESET)		/* Request was cancelled */
		usb_ep_fifo_flush(ep);

	/* Hold the lock while we update the request and buffer states */
	bh->inreq_busy = 0;
	bh->state = BUF_STATE_EMPTY;
	wakeup_thread(common);
}

static void bulk_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct fsg_common	*common = ep->driver_data;
	struct fsg_buffhd	*bh = req->context;

	dump_msg(common, "bulk-out", req->buf, req->actual);
	if (req->status || req->actual != bh->bulk_out_intended_length)
		DBG(common, "%s --> %d, %u/%u\n", __func__,
				req->status, req->actual,
				bh->bulk_out_intended_length);
	if (req->status == -ECONNRESET)		/* Request was cancelled */
		usb_ep_fifo_flush(ep);

	/* Hold the lock while we update the request and buffer states */
	bh->outreq_busy = 0;
	bh->state = BUF_STATE_FULL;
	wakeup_thread(common);
}

/*-------------------------------------------------------------------------*/

/* Ep0 class-specific handlers.  These always run in_irq. */

static int fsg_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct fsg_dev		*fsg = fsg_from_func(f);
	struct usb_request	*req = fsg->common->ep0req;
	u16			w_index = get_unaligned_le16(&ctrl->wIndex);
	u16			w_value = get_unaligned_le16(&ctrl->wValue);
	u16			w_length = get_unaligned_le16(&ctrl->wLength);

	if (!fsg_is_set(fsg->common))
		return -EOPNOTSUPP;

	switch (ctrl->bRequest) {

	case US_BULK_RESET_REQUEST:
		if (ctrl->bRequestType !=
		    (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE))
			break;
		if (w_index != fsg->interface_number || w_value != 0)
			return -EDOM;

		/* Raise an exception to stop the current operation
		 * and reinitialize our state. */
		DBG(fsg, "bulk reset request\n");
		raise_exception(fsg->common, FSG_STATE_RESET);
		return DELAYED_STATUS;

	case US_BULK_GET_MAX_LUN:
		if (ctrl->bRequestType !=
		    (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE))
			break;
		if (w_index != fsg->interface_number || w_value != 0)
			return -EDOM;
		VDBG(fsg, "get max LUN\n");
		*(u8 *) req->buf = fsg->common->nluns - 1;

		/* Respond with data/status */
		req->length = min((u16)1, w_length);
		return ep0_queue(fsg->common);
	}

	VDBG(fsg,
	     "unknown class-specific control req "
	     "%02x.%02x v%04x i%04x l%u\n",
	     ctrl->bRequestType, ctrl->bRequest,
	     get_unaligned_le16(&ctrl->wValue), w_index, w_length);
	return -EOPNOTSUPP;
}

/*-------------------------------------------------------------------------*/

/* All the following routines run in process context */

/* Use this for bulk or interrupt transfers, not ep0 */
static void start_transfer(struct fsg_dev *fsg, struct usb_ep *ep,
		struct usb_request *req, int *pbusy,
		enum fsg_buffer_state *state)
{
	int	rc;

	if (ep == fsg->bulk_in)
		dump_msg(fsg, "bulk-in", req->buf, req->length);

	*pbusy = 1;
	*state = BUF_STATE_BUSY;
	rc = usb_ep_queue(ep, req);
	if (rc != 0) {
		*pbusy = 0;
		*state = BUF_STATE_EMPTY;

		/* We can't do much more than wait for a reset */

		/* Note: currently the net2280 driver fails zero-length
		 * submissions if DMA is enabled. */
		if (rc != -ESHUTDOWN && !(rc == -EOPNOTSUPP &&
						req->length == 0))
			WARNING(fsg, "error in submission: %s --> %d\n",
					ep->name, rc);
	}
}

#define START_TRANSFER_OR(common, ep_name, req, pbusy, state)		\
	if (fsg_is_set(common))						\
		start_transfer((common)->fsg, (common)->fsg->ep_name,	\
			       req, pbusy, state);			\
	else

#define START_TRANSFER(common, ep_name, req, pbusy, state)		\
	START_TRANSFER_OR(common, ep_name, req, pbusy, state) (void)0

static int sleep_thread(struct fsg_common *common)
{
	int ret;

	/* Wait until a signal arrives or we are woken up */
	ret = wait_for_completion_interruptible(&common->thread_wakeup_needed);
	if (ret)
		return ret;

	reinit_completion(&common->thread_wakeup_needed);
	return 0;
}

/*-------------------------------------------------------------------------*/

static int do_read(struct fsg_common *common)
{
	struct fsg_lun		*curlun = &common->luns[common->lun];
	u32			lba;
	struct fsg_buffhd	*bh;
	int			rc;
	u32			amount_left;
	loff_t			file_offset;
	unsigned int		amount;
	unsigned int		partial_page;
	ssize_t			nread;

	/* Get the starting Logical Block Address and check that it's
	 * not too big */
	if (common->cmnd[0] == SCSI_READ6)
		lba = get_unaligned_be24(&common->cmnd[1]);
	else {
		lba = get_unaligned_be32(&common->cmnd[2]);

		/* We allow DPO (Disable Page Out = don't save data in the
		 * cache) and FUA (Force Unit Access = don't read from the
		 * cache), but we don't implement them. */
		if ((common->cmnd[1] & ~0x18) != 0) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}
	file_offset = ((loff_t) lba) << 9;

	/* Carry out the file reads */
	amount_left = common->data_size_from_cmnd;
	if (unlikely(amount_left == 0))
		return -EIO;		/* No default reply */

	for (;;) {
		/* Wait for the next buffer to become available */
		bh = common->next_buffhd_to_fill;
		while (bh->state != BUF_STATE_EMPTY) {
			rc = sleep_thread(common);
			if (rc)
				return rc;
		}

		/* Figure out how much we need to read:
		 * Try to read the remaining amount.
		 * But don't read more than the buffer size.
		 * And don't try to read past the end of the file.
		 * Finally, if we're not at a page boundary, don't read past
		 *	the next page.
		 * If this means reading 0 then we were asked to read past
		 *	the end of file. */
		amount = min(amount_left, FSG_BUFLEN);
		partial_page = file_offset & (PAGE_CACHE_SIZE - 1);
		if (partial_page > 0)
			amount = min(amount, (unsigned int) PAGE_CACHE_SIZE -
					partial_page);


		/* If we were asked to read past the end of file,
		 * end with an empty buffer. */
		if (amount == 0) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->info_valid = 1;
			bh->inreq->length = 0;
			bh->state = BUF_STATE_FULL;
			break;
		}

		/* Perform the read */
		nread = pread(ums[common->lun].fd, bh->buf, amount, file_offset);

		VLDBG(curlun, "file read %u @ %llu -> %zd\n", amount,
				(unsigned long long) file_offset,
				nread);
		if (nread <= 0) {
			const char *err = nread ? strerror(-nread) : "EOF";
			LDBG(curlun, "error in file read: %s\n", err);
			nread = 0;
		} else if (nread < amount) {
			LDBG(curlun, "partial file read: %d/%u\n",
					(int) nread, amount);
			nread -= (nread & 511);	/* Round down to a block */
		}
		file_offset  += nread;
		amount_left  -= nread;
		common->residue -= nread;
		bh->inreq->length = nread;
		bh->state = BUF_STATE_FULL;

		/* If an error occurred, report it and its position */
		if (nread < amount) {
			curlun->sense_data = SS_UNRECOVERED_READ_ERROR;
			curlun->info_valid = 1;
			break;
		}

		if (amount_left == 0)
			break;		/* No more left to read */

		/* Send this buffer and go read some more */
		bh->inreq->zero = 0;
		START_TRANSFER_OR(common, bulk_in, bh->inreq,
			       &bh->inreq_busy, &bh->state)
			/* Don't know what to do if
			 * common->fsg is NULL */
			return -EIO;
		common->next_buffhd_to_fill = bh->next;
	}

	return -EIO;		/* No default reply */
}

/*-------------------------------------------------------------------------*/

static int do_write(struct fsg_common *common)
{
	struct fsg_lun		*curlun = &common->luns[common->lun];
	u32			lba;
	struct fsg_buffhd	*bh;
	int			get_some_more;
	u32			amount_left_to_req, amount_left_to_write;
	loff_t			usb_offset, file_offset;
	unsigned int		amount;
	unsigned int		partial_page;
	ssize_t			nwritten;
	int			rc;

	if (curlun->ro) {
		curlun->sense_data = SS_WRITE_PROTECTED;
		return -EINVAL;
	}

	/* Get the starting Logical Block Address and check that it's
	 * not too big */
	if (common->cmnd[0] == SCSI_WRITE6)
		lba = get_unaligned_be24(&common->cmnd[1]);
	else {
		lba = get_unaligned_be32(&common->cmnd[2]);

		/* We allow DPO (Disable Page Out = don't save data in the
		 * cache) and FUA (Force Unit Access = write directly to the
		 * medium).  We don't implement DPO; we implement FUA by
		 * performing synchronous output. */
		if (common->cmnd[1] & ~0x18) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	/* Carry out the file writes */
	get_some_more = 1;
	file_offset = usb_offset = ((loff_t) lba) << 9;
	amount_left_to_req = common->data_size_from_cmnd;
	amount_left_to_write = common->data_size_from_cmnd;

	while (amount_left_to_write > 0) {

		/* Queue a request for more data from the host */
		bh = common->next_buffhd_to_fill;
		if (bh->state == BUF_STATE_EMPTY && get_some_more) {

			/* Figure out how much we want to get:
			 * Try to get the remaining amount.
			 * But don't get more than the buffer size.
			 * And don't try to go past the end of the file.
			 * If we're not at a page boundary,
			 *	don't go past the next page.
			 * If this means getting 0, then we were asked
			 *	to write past the end of file.
			 * Finally, round down to a block boundary. */
			amount = min(amount_left_to_req, FSG_BUFLEN);
			partial_page = usb_offset & (PAGE_CACHE_SIZE - 1);
			if (partial_page > 0)
				amount = min(amount,
	(unsigned int) PAGE_CACHE_SIZE - partial_page);

			if (amount == 0) {
				get_some_more = 0;
				curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
				curlun->info_valid = 1;
				continue;
			}
			amount -= (amount & 511);
			if (amount == 0) {

				/* Why were we were asked to transfer a
				 * partial block? */
				get_some_more = 0;
				continue;
			}

			/* Get the next buffer */
			usb_offset += amount;
			common->usb_amount_left -= amount;
			amount_left_to_req -= amount;
			if (amount_left_to_req == 0)
				get_some_more = 0;

			/* amount is always divisible by 512, hence by
			 * the bulk-out maxpacket size */
			bh->outreq->length = amount;
			bh->bulk_out_intended_length = amount;
			bh->outreq->short_not_ok = 1;
			START_TRANSFER_OR(common, bulk_out, bh->outreq,
					  &bh->outreq_busy, &bh->state)
				/* Don't know what to do if
				 * common->fsg is NULL */
				return -EIO;
			common->next_buffhd_to_fill = bh->next;
			continue;
		}

		/* Write the received data to the backing file */
		bh = common->next_buffhd_to_drain;
		if (bh->state == BUF_STATE_EMPTY && !get_some_more)
			break;			/* We stopped early */
		if (bh->state == BUF_STATE_FULL) {
			common->next_buffhd_to_drain = bh->next;
			bh->state = BUF_STATE_EMPTY;

			/* Did something go wrong with the transfer? */
			if (bh->outreq->status != 0) {
				curlun->sense_data = SS_COMMUNICATION_FAILURE;
				curlun->info_valid = 1;
				break;
			}

			amount = bh->outreq->actual;

			/* Perform the write */
			nwritten = pwrite(ums[common->lun].fd, bh->buf, amount, file_offset);

			VLDBG(curlun, "file write %u @ %llu -> %zd\n", amount,
					(unsigned long long) file_offset,
					nwritten);

			if (nwritten < 0) {
				LDBG(curlun, "error in file write: %pe\n", ERR_PTR(nwritten));
				nwritten = 0;
			} else if (nwritten < amount) {
				LDBG(curlun, "partial file write: %d/%u\n",
						(int) nwritten, amount);
				nwritten -= (nwritten & 511);
				/* Round down to a block */
			}
			file_offset += nwritten;
			amount_left_to_write -= nwritten;
			common->residue -= nwritten;

			/* If an error occurred, report it and its position */
			if (nwritten < amount) {
				pr_warn("nwritten:%zd amount:%u\n", nwritten,
				       amount);
				curlun->sense_data = SS_WRITE_ERROR;
				curlun->info_valid = 1;
				break;
			}

			/* Did the host decide to stop early? */
			if (bh->outreq->actual != bh->outreq->length) {
				common->short_packet_received = 1;
				break;
			}
			continue;
		}

		/* Wait for something to happen */
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	return -EIO;		/* No default reply */
}

/*-------------------------------------------------------------------------*/

static int do_synchronize_cache(struct fsg_common *common)
{
	return 0;
}

/*-------------------------------------------------------------------------*/

static int do_verify(struct fsg_common *common)
{
	struct fsg_lun		*curlun = &common->luns[common->lun];
	u32			lba;
	u32			verification_length;
	struct fsg_buffhd	*bh = common->next_buffhd_to_fill;
	loff_t			file_offset;
	u32			amount_left;
	unsigned int		amount;
	ssize_t			nread;

	/* Get the starting Logical Block Address and check that it's
	 * not too big */
	lba = get_unaligned_be32(&common->cmnd[2]);
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	/* We allow DPO (Disable Page Out = don't save data in the
	 * cache) but we don't implement it. */
	if (common->cmnd[1] & ~0x10) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	verification_length = get_unaligned_be16(&common->cmnd[7]);
	if (unlikely(verification_length == 0))
		return -EIO;		/* No default reply */

	/* Prepare to carry out the file verify */
	amount_left = verification_length << 9;
	file_offset = ((loff_t) lba) << 9;

	/* Write out all the dirty buffers before invalidating them */

	/* Just try to read the requested blocks */
	while (amount_left > 0) {

		/* Figure out how much we need to read:
		 * Try to read the remaining amount, but not more than
		 * the buffer size.
		 * And don't try to read past the end of the file.
		 * If this means reading 0 then we were asked to read
		 * past the end of file. */
		amount = min(amount_left, FSG_BUFLEN);
		if (amount == 0) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->info_valid = 1;
			break;
		}

		/* Perform the read */
		nread = pread(ums[common->lun].fd, bh->buf, amount, file_offset);

		VLDBG(curlun, "file read %u @ %llu -> %zd\n", amount,
				(unsigned long long) file_offset,
				nread);
		if (nread <= 0) {
			const char *err = nread ? strerror(-nread) : "EOF";
			LDBG(curlun, "error in file read: %s\n", err);
			nread = 0;
		} else if (nread < amount) {
			LDBG(curlun, "partial file verify: %d/%u\n",
					(int) nread, amount);
			nread -= (nread & 511);	/* Round down to a sector */
		}
		if (nread == 0) {
			curlun->sense_data = SS_UNRECOVERED_READ_ERROR;
			curlun->info_valid = 1;
			break;
		}
		file_offset += nread;
		amount_left -= nread;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

static int do_inquiry(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun *curlun = &common->luns[common->lun];
	static const char vendor_id[] = "Linux   ";
	u8	*buf = (u8 *) bh->buf;

	if (!curlun) {		/* Unsupported LUNs are okay */
		common->bad_lun_okay = 1;
		memset(buf, 0, 36);
		buf[0] = 0x7f;		/* Unsupported, no device-type */
		buf[4] = 31;		/* Additional length */
		return 36;
	}

	memset(buf, 0, 8);
	buf[0] = TYPE_DISK;
	buf[1] = curlun->removable ? 0x80 : 0;
	buf[2] = 2;		/* ANSI SCSI level 2 */
	buf[3] = 2;		/* SCSI-2 INQUIRY data format */
	buf[4] = 31;		/* Additional length */
				/* No special options */
	sprintf((char *) (buf + 8), "%-8s%-16s%04x", (char*) vendor_id ,
			ums[common->lun].name, (u16) 0xffff);

	return 36;
}


static int do_request_sense(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];
	u8		*buf = (u8 *) bh->buf;
	u32		sd, sdinfo = 0;
	int		valid;

	/*
	 * From the SCSI-2 spec., section 7.9 (Unit attention condition):
	 *
	 * If a REQUEST SENSE command is received from an initiator
	 * with a pending unit attention condition (before the target
	 * generates the contingent allegiance condition), then the
	 * target shall either:
	 *   a) report any pending sense data and preserve the unit
	 *	attention condition on the logical unit, or,
	 *   b) report the unit attention condition, may discard any
	 *	pending sense data, and clear the unit attention
	 *	condition on the logical unit for that initiator.
	 *
	 * FSG normally uses option a); enable this code to use option b).
	 */
#if 0
	if (curlun && curlun->unit_attention_data != SS_NO_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;
	}
#endif

	if (!curlun) {		/* Unsupported LUNs are okay */
		common->bad_lun_okay = 1;
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;
		valid = 0;
	} else {
		sd = curlun->sense_data;
		valid = curlun->info_valid << 7;
		curlun->sense_data = SS_NO_SENSE;
		curlun->info_valid = 0;
	}

	memset(buf, 0, 18);
	buf[0] = valid | 0x70;			/* Valid, current error */
	buf[2] = SK(sd);
	put_unaligned_be32(sdinfo, &buf[3]);	/* Sense information */
	buf[7] = 18 - 8;			/* Additional sense length */
	buf[12] = ASC(sd);
	buf[13] = ASCQ(sd);
	return 18;
}

static int do_read_capacity(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];
	u32		lba = get_unaligned_be32(&common->cmnd[2]);
	int		pmi = common->cmnd[8];
	u8		*buf = (u8 *) bh->buf;

	/* Check the PMI and LBA fields */
	if (pmi > 1 || (pmi == 0 && lba != 0)) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	put_unaligned_be32(curlun->num_sectors - 1, &buf[0]);
						/* Max logical block */
	put_unaligned_be32(512, &buf[4]);	/* Block length */
	return 8;
}

static int do_read_header(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];
	int		msf = common->cmnd[1] & 0x02;
	u32		lba = get_unaligned_be32(&common->cmnd[2]);
	u8		*buf = (u8 *) bh->buf;

	if (common->cmnd[1] & ~0x02) {		/* Mask away MSF */
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	memset(buf, 0, 8);
	buf[0] = 0x01;		/* 2048 bytes of user data, rest is EC */
	store_cdrom_address(&buf[4], msf, lba);
	return 8;
}


static int do_read_toc(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];
	int		msf = common->cmnd[1] & 0x02;
	int		start_track = common->cmnd[6];
	u8		*buf = (u8 *) bh->buf;

	if ((common->cmnd[1] & ~0x02) != 0 ||	/* Mask away MSF */
			start_track > 1) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	memset(buf, 0, 20);
	buf[1] = (20-2);		/* TOC data length */
	buf[2] = 1;			/* First track number */
	buf[3] = 1;			/* Last track number */
	buf[5] = 0x16;			/* Data track, copying allowed */
	buf[6] = 0x01;			/* Only track is number 1 */
	store_cdrom_address(&buf[8], msf, 0);

	buf[13] = 0x16;			/* Lead-out track is data */
	buf[14] = 0xAA;			/* Lead-out track number */
	store_cdrom_address(&buf[16], msf, curlun->num_sectors);

	return 20;
}

static int do_mode_sense(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];
	int		mscmnd = common->cmnd[0];
	u8		*buf = (u8 *) bh->buf;
	u8		*buf0 = buf;
	int		pc, page_code;
	int		changeable_values, all_pages;
	int		valid_page = 0;
	int		len, limit;

	if ((common->cmnd[1] & ~0x08) != 0) {	/* Mask away DBD */
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}
	pc = common->cmnd[2] >> 6;
	page_code = common->cmnd[2] & 0x3f;
	if (pc == 3) {
		curlun->sense_data = SS_SAVING_PARAMETERS_NOT_SUPPORTED;
		return -EINVAL;
	}
	changeable_values = (pc == 1);
	all_pages = (page_code == 0x3f);

	/* Write the mode parameter header.  Fixed values are: default
	 * medium type, no cache control (DPOFUA), and no block descriptors.
	 * The only variable value is the WriteProtect bit.  We will fill in
	 * the mode data length later. */
	memset(buf, 0, 8);
	if (mscmnd == SCSI_MODE_SEN6) {
		buf[2] = (curlun->ro ? 0x80 : 0x00);		/* WP, DPOFUA */
		buf += 4;
		limit = 255;
	} else {			/* SCSI_MODE_SEN10 */
		buf[3] = (curlun->ro ? 0x80 : 0x00);		/* WP, DPOFUA */
		buf += 8;
		limit = 65535;		/* Should really be FSG_BUFLEN */
	}

	/* No block descriptors */

	/* The mode pages, in numerical order.  The only page we support
	 * is the Caching page. */
	if (page_code == 0x08 || all_pages) {
		valid_page = 1;
		buf[0] = 0x08;		/* Page code */
		buf[1] = 10;		/* Page length */
		memset(buf+2, 0, 10);	/* None of the fields are changeable */

		if (!changeable_values) {
			buf[2] = 0x04;	/* Write cache enable, */
					/* Read cache not disabled */
					/* No cache retention priorities */
			put_unaligned_be16(0xffff, &buf[4]);
					/* Don't disable prefetch */
					/* Minimum prefetch = 0 */
			put_unaligned_be16(0xffff, &buf[8]);
					/* Maximum prefetch */
			put_unaligned_be16(0xffff, &buf[10]);
					/* Maximum prefetch ceiling */
		}
		buf += 12;
	}

	/* Check that a valid page was requested and the mode data length
	 * isn't too long. */
	len = buf - buf0;
	if (!valid_page || len > limit) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	/*  Store the mode data length */
	if (mscmnd == SCSI_MODE_SEN6)
		buf0[0] = len - 1;
	else
		put_unaligned_be16(len - 2, buf0);
	return len;
}


static int do_start_stop(struct fsg_common *common)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];

	if (!curlun) {
		return -EINVAL;
	} else if (!curlun->removable) {
		curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	}

	return 0;
}

static int do_prevent_allow(struct fsg_common *common)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];
	int		prevent;

	if (!curlun->removable) {
		curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	}

	prevent = common->cmnd[4] & 0x01;
	if ((common->cmnd[4] & ~0x01) != 0) {	/* Mask away Prevent */
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	if (curlun->prevent_medium_removal && !prevent)
		fsg_lun_fsync_sub(curlun);
	curlun->prevent_medium_removal = prevent;
	return 0;
}


static int do_read_format_capacities(struct fsg_common *common,
			struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];
	u8		*buf = (u8 *) bh->buf;

	buf[0] = buf[1] = buf[2] = 0;
	buf[3] = 8;	/* Only the Current/Maximum Capacity Descriptor */
	buf += 4;

	put_unaligned_be32(curlun->num_sectors, &buf[0]);
						/* Number of blocks */
	put_unaligned_be32(512, &buf[4]);	/* Block length */
	buf[4] = 0x02;				/* Current capacity */
	return 12;
}


static int do_mode_select(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = &common->luns[common->lun];

	/* We don't support MODE SELECT */
	if (curlun)
		curlun->sense_data = SS_INVALID_COMMAND;
	return -EINVAL;
}


/*-------------------------------------------------------------------------*/

static int halt_bulk_in_endpoint(struct fsg_dev *fsg)
{
	int	rc;

	rc = fsg_set_halt(fsg, fsg->bulk_in);
	if (rc == -EAGAIN)
		VDBG(fsg, "delayed bulk-in endpoint halt\n");
	while (rc != 0) {
		if (rc != -EAGAIN) {
			WARNING(fsg, "usb_ep_set_halt -> %d\n", rc);
			rc = 0;
			break;
		}

		rc = usb_ep_set_halt(fsg->bulk_in);
	}
	return rc;
}

static int wedge_bulk_in_endpoint(struct fsg_dev *fsg)
{
	int	rc;

	DBG(fsg, "bulk-in set wedge\n");
	rc = 0; /* usb_ep_set_wedge(fsg->bulk_in); */
	if (rc == -EAGAIN)
		VDBG(fsg, "delayed bulk-in endpoint wedge\n");
	while (rc != 0) {
		if (rc != -EAGAIN) {
			WARNING(fsg, "usb_ep_set_wedge -> %d\n", rc);
			rc = 0;
			break;
		}
	}
	return rc;
}

static int pad_with_zeros(struct fsg_dev *fsg)
{
	struct fsg_buffhd	*bh = fsg->common->next_buffhd_to_fill;
	u32			nkeep = bh->inreq->length;
	u32			nsend;
	int			rc;

	bh->state = BUF_STATE_EMPTY;		/* For the first iteration */
	fsg->common->usb_amount_left = nkeep + fsg->common->residue;
	while (fsg->common->usb_amount_left > 0) {

		/* Wait for the next buffer to be free */
		while (bh->state != BUF_STATE_EMPTY) {
			rc = sleep_thread(fsg->common);
			if (rc)
				return rc;
		}

		nsend = min(fsg->common->usb_amount_left, FSG_BUFLEN);
		memset(bh->buf + nkeep, 0, nsend - nkeep);
		bh->inreq->length = nsend;
		bh->inreq->zero = 0;
		start_transfer(fsg, fsg->bulk_in, bh->inreq,
				&bh->inreq_busy, &bh->state);
		bh = fsg->common->next_buffhd_to_fill = bh->next;
		fsg->common->usb_amount_left -= nsend;
		nkeep = 0;
	}
	return 0;
}

static int throw_away_data(struct fsg_common *common)
{
	struct fsg_buffhd	*bh;
	u32			amount;
	int			rc;

	for (bh = common->next_buffhd_to_drain;
	     bh->state != BUF_STATE_EMPTY || common->usb_amount_left > 0;
	     bh = common->next_buffhd_to_drain) {

		/* Throw away the data in a filled buffer */
		if (bh->state == BUF_STATE_FULL) {
			bh->state = BUF_STATE_EMPTY;
			common->next_buffhd_to_drain = bh->next;

			/* A short packet or an error ends everything */
			if (bh->outreq->actual != bh->outreq->length ||
					bh->outreq->status != 0) {
				raise_exception(common,
						FSG_STATE_ABORT_BULK_OUT);
				return -EPIPE;
			}
			continue;
		}

		/* Try to submit another request if we need one */
		bh = common->next_buffhd_to_fill;
		if (bh->state == BUF_STATE_EMPTY
		 && common->usb_amount_left > 0) {
			amount = min(common->usb_amount_left, FSG_BUFLEN);

			/* amount is always divisible by 512, hence by
			 * the bulk-out maxpacket size */
			bh->outreq->length = amount;
			bh->bulk_out_intended_length = amount;
			bh->outreq->short_not_ok = 1;
			START_TRANSFER_OR(common, bulk_out, bh->outreq,
					  &bh->outreq_busy, &bh->state)
				/* Don't know what to do if
				 * common->fsg is NULL */
				return -EIO;
			common->next_buffhd_to_fill = bh->next;
			common->usb_amount_left -= amount;
			continue;
		}

		/* Otherwise wait for something to happen */
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}
	return 0;
}


static int finish_reply(struct fsg_common *common)
{
	struct fsg_buffhd	*bh = common->next_buffhd_to_fill;
	int			rc = 0;

	switch (common->data_dir) {
	case DATA_DIR_NONE:
		break;			/* Nothing to send */

	/* If we don't know whether the host wants to read or write,
	 * this must be CB or CBI with an unknown command.  We mustn't
	 * try to send or receive any data.  So stall both bulk pipes
	 * if we can and wait for a reset. */
	case DATA_DIR_UNKNOWN:
		if (!common->can_stall) {
			/* Nothing */
		} else if (fsg_is_set(common)) {
			fsg_set_halt(common->fsg, common->fsg->bulk_out);
			rc = halt_bulk_in_endpoint(common->fsg);
		} else {
			/* Don't know what to do if common->fsg is NULL */
			rc = -EIO;
		}
		break;

	/* All but the last buffer of data must have already been sent */
	case DATA_DIR_TO_HOST:
		if (common->data_size == 0) {
			/* Nothing to send */

		/* If there's no residue, simply send the last buffer */
		} else if (common->residue == 0) {
			bh->inreq->zero = 0;
			START_TRANSFER_OR(common, bulk_in, bh->inreq,
					  &bh->inreq_busy, &bh->state)
				return -EIO;
			common->next_buffhd_to_fill = bh->next;

		/* For Bulk-only, if we're allowed to stall then send the
		 * short packet and halt the bulk-in endpoint.  If we can't
		 * stall, pad out the remaining data with 0's. */
		} else if (common->can_stall) {
			bh->inreq->zero = 1;
			START_TRANSFER_OR(common, bulk_in, bh->inreq,
					  &bh->inreq_busy, &bh->state)
				/* Don't know what to do if
				 * common->fsg is NULL */
				rc = -EIO;
			common->next_buffhd_to_fill = bh->next;
			if (common->fsg)
				rc = halt_bulk_in_endpoint(common->fsg);
		} else if (fsg_is_set(common)) {
			rc = pad_with_zeros(common->fsg);
		} else {
			/* Don't know what to do if common->fsg is NULL */
			rc = -EIO;
		}
		break;

	/* We have processed all we want from the data the host has sent.
	 * There may still be outstanding bulk-out requests. */
	case DATA_DIR_FROM_HOST:
		if (common->residue == 0) {
			/* Nothing to receive */

		/* Did the host stop sending unexpectedly early? */
		} else if (common->short_packet_received) {
			raise_exception(common, FSG_STATE_ABORT_BULK_OUT);
			rc = -EPIPE;

		/* We haven't processed all the incoming data.  Even though
		 * we may be allowed to stall, doing so would cause a race.
		 * The controller may already have ACK'ed all the remaining
		 * bulk-out packets, in which case the host wouldn't see a
		 * STALL.  Not realizing the endpoint was halted, it wouldn't
		 * clear the halt -- leading to problems later on. */
#if 0
		} else if (common->can_stall) {
			if (fsg_is_set(common))
				fsg_set_halt(common->fsg,
					     common->fsg->bulk_out);
			raise_exception(common, FSG_STATE_ABORT_BULK_OUT);
			rc = -EPIPE;
#endif

		/* We can't stall.  Read in the excess data and throw it
		 * all away. */
		} else {
			rc = throw_away_data(common);
		}
		break;
	}
	return rc;
}


static int send_status(struct fsg_common *common)
{
	struct fsg_lun		*curlun = &common->luns[common->lun];
	struct fsg_buffhd	*bh;
	struct bulk_cs_wrap	*csw;
	int			rc;
	u8			status = US_BULK_STAT_OK;
	u32			sd, sdinfo = 0;

	/* Wait for the next buffer to become available */
	bh = common->next_buffhd_to_fill;
	while (bh->state != BUF_STATE_EMPTY) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	if (curlun)
		sd = curlun->sense_data;
	else if (common->bad_lun_okay)
		sd = SS_NO_SENSE;
	else
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;

	if (common->phase_error) {
		DBG(common, "sending phase-error status\n");
		status = US_BULK_STAT_PHASE;
		sd = SS_INVALID_COMMAND;
	} else if (sd != SS_NO_SENSE) {
		DBG(common, "sending command-failure status\n");
		status = US_BULK_STAT_FAIL;
		VDBG(common, "  sense data: SK x%02x, ASC x%02x, ASCQ x%02x;"
			"  info x%x\n",
			SK(sd), ASC(sd), ASCQ(sd), sdinfo);
	}

	/* Store and send the Bulk-only CSW */
	csw = (void *)bh->buf;

	csw->Signature = cpu_to_le32(US_BULK_CS_SIGN);
	csw->Tag = common->tag;
	csw->Residue = cpu_to_le32(common->residue);
	csw->Status = status;

	bh->inreq->length = US_BULK_CS_WRAP_LEN;
	bh->inreq->zero = 0;
	START_TRANSFER_OR(common, bulk_in, bh->inreq,
			  &bh->inreq_busy, &bh->state)
		/* Don't know what to do if common->fsg is NULL */
		return -EIO;

	common->next_buffhd_to_fill = bh->next;
	return 0;
}


/*-------------------------------------------------------------------------*/

/* Check whether the command is properly formed and whether its data size
 * and direction agree with the values we already have. */
static int check_command(struct fsg_common *common, int cmnd_size,
		enum data_direction data_dir, unsigned int mask,
		int needs_medium, const char *name)
{
	int			i;
	int			lun = common->cmnd[1] >> 5;
	static const char	dirletter[4] = {'u', 'o', 'i', 'n'};
	char			hdlen[20];
	struct fsg_lun		*curlun;

	hdlen[0] = 0;
	if (common->data_dir != DATA_DIR_UNKNOWN)
		sprintf(hdlen, ", H%c=%u", dirletter[(int) common->data_dir],
				common->data_size);
	VDBG(common, "SCSI command: %s;  Dc=%d, D%c=%u;  Hc=%d%s\n",
	     name, cmnd_size, dirletter[(int) data_dir],
	     common->data_size_from_cmnd, common->cmnd_size, hdlen);

	/* We can't reply at all until we know the correct data direction
	 * and size. */
	if (common->data_size_from_cmnd == 0)
		data_dir = DATA_DIR_NONE;
	if (common->data_size < common->data_size_from_cmnd) {
		/* Host data size < Device data size is a phase error.
		 * Carry out the command, but only transfer as much as
		 * we are allowed. */
		common->data_size_from_cmnd = common->data_size;
		common->phase_error = 1;
	}
	common->residue = common->data_size;
	common->usb_amount_left = common->data_size;

	/* Conflicting data directions is a phase error */
	if (common->data_dir != data_dir
	 && common->data_size_from_cmnd > 0) {
		common->phase_error = 1;
		return -EINVAL;
	}

	/* Verify the length of the command itself */
	if (cmnd_size != common->cmnd_size) {

		/* Special case workaround: There are plenty of buggy SCSI
		 * implementations. Many have issues with cbw->Length
		 * field passing a wrong command size. For those cases we
		 * always try to work around the problem by using the length
		 * sent by the host side provided it is at least as large
		 * as the correct command length.
		 * Examples of such cases would be MS-Windows, which issues
		 * REQUEST SENSE with cbw->Length == 12 where it should
		 * be 6, and xbox360 issuing INQUIRY, TEST UNIT READY and
		 * REQUEST SENSE with cbw->Length == 10 where it should
		 * be 6 as well.
		 */
		if (cmnd_size <= common->cmnd_size) {
			DBG(common, "%s is buggy! Expected length %d "
			    "but we got %d\n", name,
			    cmnd_size, common->cmnd_size);
			cmnd_size = common->cmnd_size;
		} else {
			common->phase_error = 1;
			return -EINVAL;
		}
	}

	/* Check that the LUN values are consistent */
	if (common->lun != lun)
		DBG(common, "using LUN %d from CBW, not LUN %d from CDB\n",
		    common->lun, lun);

	/* Check the LUN */
	if (common->lun < common->nluns) {
		curlun = &common->luns[common->lun];
		if (common->cmnd[0] != SCSI_REQ_SENSE) {
			curlun->sense_data = SS_NO_SENSE;
			curlun->info_valid = 0;
		}
	} else {
		curlun = NULL;
		common->bad_lun_okay = 0;

		/* INQUIRY and REQUEST SENSE commands are explicitly allowed
		 * to use unsupported LUNs; all others may not. */
		if (common->cmnd[0] != SCSI_INQUIRY &&
		    common->cmnd[0] != SCSI_REQ_SENSE) {
			DBG(common, "unsupported LUN %d\n", common->lun);
			return -EINVAL;
		}
	}
#if 0
	/* If a unit attention condition exists, only INQUIRY and
	 * REQUEST SENSE commands are allowed; anything else must fail. */
	if (curlun && curlun->unit_attention_data != SS_NO_SENSE &&
			common->cmnd[0] != SCSI_INQUIRY &&
			common->cmnd[0] != SCSI_REQ_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;
		return -EINVAL;
	}
#endif
	/* Check that only command bytes listed in the mask are non-zero */
	common->cmnd[1] &= 0x1f;			/* Mask away the LUN */
	for (i = 1; i < cmnd_size; ++i) {
		if (common->cmnd[i] && !(mask & (1 << i))) {
			if (curlun)
				curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}

	return 0;
}

static int do_scsi_command(struct fsg_common *common)
{
	struct fsg_buffhd	*bh;
	int			rc;
	int			reply = -EINVAL;
	int			i;
	static char		unknown[16];
	struct fsg_lun		*curlun = &common->luns[common->lun];

	dump_cdb(common);

	/* Wait for the next buffer to become available for data or status */
	bh = common->next_buffhd_to_fill;
	common->next_buffhd_to_drain = bh;
	while (bh->state != BUF_STATE_EMPTY) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}
	common->phase_error = 0;
	common->short_packet_received = 0;

	down_read(&common->filesem);	/* We're using the backing file */
	switch (common->cmnd[0]) {

	case SCSI_INQUIRY:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_TO_HOST,
				      (1<<4), 0,
				      "INQUIRY");
		if (reply == 0)
			reply = do_inquiry(common, bh);
		break;

	case SCSI_MODE_SEL6:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_FROM_HOST,
				      (1<<1) | (1<<4), 0,
				      "MODE SELECT(6)");
		if (reply == 0)
			reply = do_mode_select(common, bh);
		break;

	case SCSI_MODE_SEL10:
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_FROM_HOST,
				      (1<<1) | (3<<7), 0,
				      "MODE SELECT(10)");
		if (reply == 0)
			reply = do_mode_select(common, bh);
		break;

	case SCSI_MODE_SEN6:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_TO_HOST,
				      (1<<1) | (1<<2) | (1<<4), 0,
				      "MODE SENSE(6)");
		if (reply == 0)
			reply = do_mode_sense(common, bh);
		break;

	case SCSI_MODE_SEN10:
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (1<<1) | (1<<2) | (3<<7), 0,
				      "MODE SENSE(10)");
		if (reply == 0)
			reply = do_mode_sense(common, bh);
		break;

	case SCSI_MED_REMOVL:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 6, DATA_DIR_NONE,
				      (1<<4), 0,
				      "PREVENT-ALLOW MEDIUM REMOVAL");
		if (reply == 0)
			reply = do_prevent_allow(common);
		break;

	case SCSI_READ6:
		i = common->cmnd[4];
		common->data_size_from_cmnd = (i == 0 ? 256 : i) << 9;
		reply = check_command(common, 6, DATA_DIR_TO_HOST,
				      (7<<1) | (1<<4), 1,
				      "READ(6)");
		if (reply == 0)
			reply = do_read(common);
		break;

	case SCSI_READ10:
		common->data_size_from_cmnd =
				get_unaligned_be16(&common->cmnd[7]) << 9;
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (1<<1) | (0xf<<2) | (3<<7), 1,
				      "READ(10)");
		if (reply == 0)
			reply = do_read(common);
		break;

	case SCSI_READ12:
		common->data_size_from_cmnd =
				get_unaligned_be32(&common->cmnd[6]) << 9;
		reply = check_command(common, 12, DATA_DIR_TO_HOST,
				      (1<<1) | (0xf<<2) | (0xf<<6), 1,
				      "READ(12)");
		if (reply == 0)
			reply = do_read(common);
		break;

	case SCSI_RD_CAPAC:
		common->data_size_from_cmnd = 8;
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (0xf<<2) | (1<<8), 1,
				      "READ CAPACITY");
		if (reply == 0)
			reply = do_read_capacity(common, bh);
		break;

	case SCSI_RD_HEADER:
		if (!common->luns[common->lun].cdrom)
			goto unknown_cmnd;
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (3<<7) | (0x1f<<1), 1,
				      "READ HEADER");
		if (reply == 0)
			reply = do_read_header(common, bh);
		break;

	case SCSI_RD_TOC:
		if (!common->luns[common->lun].cdrom)
			goto unknown_cmnd;
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (7<<6) | (1<<1), 1,
				      "READ TOC");
		if (reply == 0)
			reply = do_read_toc(common, bh);
		break;

	case SCSI_RD_FMT_CAPAC:
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (3<<7), 1,
				      "READ FORMAT CAPACITIES");
		if (reply == 0)
			reply = do_read_format_capacities(common, bh);
		break;

	case SCSI_REQ_SENSE:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_TO_HOST,
				      (1<<4), 0,
				      "REQUEST SENSE");
		if (reply == 0)
			reply = do_request_sense(common, bh);
		break;

	case SCSI_START_STP:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 6, DATA_DIR_NONE,
				      (1<<1) | (1<<4), 0,
				      "START-STOP UNIT");
		if (reply == 0)
			reply = do_start_stop(common);
		break;

	case SCSI_SYNC_CACHE:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 10, DATA_DIR_NONE,
				      (0xf<<2) | (3<<7), 1,
				      "SYNCHRONIZE CACHE");
		if (reply == 0)
			reply = do_synchronize_cache(common);
		break;

	case SCSI_TST_U_RDY:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 6, DATA_DIR_NONE,
				0, 1,
				"TEST UNIT READY");
		break;

	/* Although optional, this command is used by MS-Windows.  We
	 * support a minimal version: BytChk must be 0. */
	case SCSI_VERIFY:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 10, DATA_DIR_NONE,
				      (1<<1) | (0xf<<2) | (3<<7), 1,
				      "VERIFY");
		if (reply == 0)
			reply = do_verify(common);
		break;

	case SCSI_WRITE6:
		i = common->cmnd[4];
		common->data_size_from_cmnd = (i == 0 ? 256 : i) << 9;
		reply = check_command(common, 6, DATA_DIR_FROM_HOST,
				      (7<<1) | (1<<4), 1,
				      "WRITE(6)");
		if (reply == 0)
			reply = do_write(common);
		break;

	case SCSI_WRITE10:
		common->data_size_from_cmnd =
				get_unaligned_be16(&common->cmnd[7]) << 9;
		reply = check_command(common, 10, DATA_DIR_FROM_HOST,
				      (1<<1) | (0xf<<2) | (3<<7), 1,
				      "WRITE(10)");
		if (reply == 0)
			reply = do_write(common);
		break;

	case SCSI_WRITE12:
		common->data_size_from_cmnd =
				get_unaligned_be32(&common->cmnd[6]) << 9;
		reply = check_command(common, 12, DATA_DIR_FROM_HOST,
				      (1<<1) | (0xf<<2) | (0xf<<6), 1,
				      "WRITE(12)");
		if (reply == 0)
			reply = do_write(common);
		break;

	/* Some mandatory commands that we recognize but don't implement.
	 * They don't mean much in this setting.  It's left as an exercise
	 * for anyone interested to implement RESERVE and RELEASE in terms
	 * of Posix locks. */
	case SCSI_FORMAT:
	case SCSI_RELEASE:
	case SCSI_RESERVE:
	case SCSI_SEND_DIAG:
		/* Fall through */

	default:
unknown_cmnd:
		common->data_size_from_cmnd = 0;
		sprintf(unknown, "Unknown x%02x", common->cmnd[0]);
		reply = check_command(common, common->cmnd_size,
				      DATA_DIR_UNKNOWN, 0xff, 0, unknown);
		if (reply == 0) {
			curlun->sense_data = SS_INVALID_COMMAND;
			reply = -EINVAL;
		}
		break;
	}
	up_read(&common->filesem);

	if (reply == -EPIPE)
		return -EPIPE;

	/* Set up the single reply buffer for finish_reply() */
	if (reply == -EINVAL)
		reply = 0;		/* Error reply length */
	if (reply >= 0 && common->data_dir == DATA_DIR_TO_HOST) {
		reply = min((u32) reply, common->data_size_from_cmnd);
		bh->inreq->length = reply;
		bh->state = BUF_STATE_FULL;
		common->residue -= reply;
	}				/* Otherwise it's already set */

	return 0;
}

/*-------------------------------------------------------------------------*/

static int received_cbw(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	struct usb_request	*req = bh->outreq;
	struct bulk_cb_wrap	*cbw = req->buf;
	struct fsg_common	*common = fsg->common;

	/* Was this a real packet?  Should it be ignored? */
	if (req->status || test_bit(IGNORE_BULK_OUT, &fsg->atomic_bitflags))
		return -EINVAL;

	/* Is the CBW valid? */
	if (req->actual != US_BULK_CB_WRAP_LEN ||
			cbw->Signature != cpu_to_le32(
				US_BULK_CB_SIGN)) {
		DBG(fsg, "invalid CBW: len %u sig 0x%x\n",
				req->actual,
				le32_to_cpu(cbw->Signature));

		/* The Bulk-only spec says we MUST stall the IN endpoint
		 * (6.6.1), so it's unavoidable.  It also says we must
		 * retain this state until the next reset, but there's
		 * no way to tell the controller driver it should ignore
		 * Clear-Feature(HALT) requests.
		 *
		 * We aren't required to halt the OUT endpoint; instead
		 * we can simply accept and discard any data received
		 * until the next reset. */
		wedge_bulk_in_endpoint(fsg);
		set_bit(IGNORE_BULK_OUT, &fsg->atomic_bitflags);
		return -EINVAL;
	}

	/* Is the CBW meaningful? */
	if (cbw->Lun >= FSG_MAX_LUNS || cbw->Flags & ~US_BULK_FLAG_IN ||
			cbw->Length <= 0 || cbw->Length > MAX_COMMAND_SIZE) {
		DBG(fsg, "non-meaningful CBW: lun = %u, flags = 0x%x, "
				"cmdlen %u\n",
				cbw->Lun, cbw->Flags, cbw->Length);

		/* We can do anything we want here, so let's stall the
		 * bulk pipes if we are allowed to. */
		if (common->can_stall) {
			fsg_set_halt(fsg, fsg->bulk_out);
			halt_bulk_in_endpoint(fsg);
		}
		return -EINVAL;
	}

	/* Save the command for later */
	common->cmnd_size = cbw->Length;
	memcpy(common->cmnd, cbw->CDB, common->cmnd_size);
	if (cbw->Flags & US_BULK_FLAG_IN)
		common->data_dir = DATA_DIR_TO_HOST;
	else
		common->data_dir = DATA_DIR_FROM_HOST;
	common->data_size = le32_to_cpu(cbw->DataTransferLength);
	if (common->data_size == 0)
		common->data_dir = DATA_DIR_NONE;
	common->lun = cbw->Lun;
	common->tag = cbw->Tag;
	return 0;
}


static int get_next_command(struct fsg_common *common)
{
	struct fsg_buffhd	*bh;
	int			rc = 0;

	/* Wait for the next buffer to become available */
	bh = common->next_buffhd_to_fill;
	while (bh->state != BUF_STATE_EMPTY) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	/* Queue a request to read a Bulk-only CBW */
	set_bulk_out_req_length(common, bh, US_BULK_CB_WRAP_LEN);
	bh->outreq->short_not_ok = 1;
	START_TRANSFER_OR(common, bulk_out, bh->outreq,
			  &bh->outreq_busy, &bh->state)
		/* Don't know what to do if common->fsg is NULL */
		return -EIO;

	/* We will drain the buffer in software, which means we
	 * can reuse it for the next filling.  No need to advance
	 * next_buffhd_to_fill. */

	/* Wait for the CBW to arrive */
	while (bh->state != BUF_STATE_FULL) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	rc = fsg_is_set(common) ? received_cbw(common->fsg, bh) : -EIO;
	bh->state = BUF_STATE_EMPTY;

	return rc;
}


/*-------------------------------------------------------------------------*/

static int enable_endpoint(struct fsg_common *common, struct usb_ep *ep)
{
	int	rc;

	ep->driver_data = common;
	rc = usb_ep_enable(ep);
	if (rc)
		ERROR(common, "can't enable %s, result %d\n", ep->name, rc);
	return rc;
}

static int alloc_request(struct fsg_common *common, struct usb_ep *ep,
		struct usb_request **preq)
{
	*preq = usb_ep_alloc_request(ep);
	if (*preq)
		return 0;
	ERROR(common, "can't allocate request for %s\n", ep->name);
	return -ENOMEM;
}

/* Reset interface setting and re-init endpoint state (toggle etc). */
static int do_set_interface(struct fsg_common *common, struct fsg_dev *new_fsg)
{
	struct fsg_dev *fsg;
	int i, rc = 0;

	if (common->running)
		DBG(common, "reset interface\n");

reset:
	/* Deallocate the requests */
	if (common->fsg) {
		fsg = common->fsg;

		for (i = 0; i < FSG_NUM_BUFFERS; ++i) {
			struct fsg_buffhd *bh = &common->buffhds[i];

			if (bh->inreq) {
				usb_ep_free_request(fsg->bulk_in, bh->inreq);
				bh->inreq = NULL;
			}
			if (bh->outreq) {
				usb_ep_free_request(fsg->bulk_out, bh->outreq);
				bh->outreq = NULL;
			}
		}

		/* Disable the endpoints */
		if (fsg->bulk_in_enabled) {
			usb_ep_disable(fsg->bulk_in);
			fsg->bulk_in_enabled = 0;
		}
		if (fsg->bulk_out_enabled) {
			usb_ep_disable(fsg->bulk_out);
			fsg->bulk_out_enabled = 0;
		}

		common->fsg = NULL;
	}

	common->running = 0;
	if (!new_fsg || rc)
		return rc;

	common->fsg = new_fsg;
	fsg = common->fsg;

	/* Enable the endpoints */
	rc = config_ep_by_speed(common->gadget, &(fsg->function), fsg->bulk_in);
	if (rc)
		goto reset;
	rc = enable_endpoint(common, fsg->bulk_in);
	if (rc)
		goto reset;
	fsg->bulk_in_enabled = 1;

	rc = config_ep_by_speed(common->gadget, &(fsg->function),
				fsg->bulk_out);
	if (rc)
		goto reset;
	rc = enable_endpoint(common, fsg->bulk_out);
	if (rc)
		goto reset;
	fsg->bulk_out_enabled = 1;
	common->bulk_out_maxpacket = 512;
	clear_bit(IGNORE_BULK_OUT, &fsg->atomic_bitflags);

	/* Allocate the requests */
	for (i = 0; i < FSG_NUM_BUFFERS; ++i) {
		struct fsg_buffhd	*bh = &common->buffhds[i];

		rc = alloc_request(common, fsg->bulk_in, &bh->inreq);
		if (rc)
			goto reset;
		rc = alloc_request(common, fsg->bulk_out, &bh->outreq);
		if (rc)
			goto reset;
		bh->inreq->buf = bh->outreq->buf = bh->buf;
		bh->inreq->context = bh->outreq->context = bh;
		bh->inreq->complete = bulk_in_complete;
		bh->outreq->complete = bulk_out_complete;
	}

	common->running = 1;

	return rc;
}


/****************************** ALT CONFIGS ******************************/


static int fsg_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct fsg_dev *fsg = fsg_from_func(f);
	fsg->common->new_fsg = fsg;
	raise_exception(fsg->common, FSG_STATE_CONFIG_CHANGE);
	return 0;
}

static void fsg_disable(struct usb_function *f)
{
	struct fsg_dev *fsg = fsg_from_func(f);
	fsg->common->new_fsg = NULL;
	raise_exception(fsg->common, FSG_STATE_CONFIG_CHANGE);
}

/*-------------------------------------------------------------------------*/

static void handle_exception(struct fsg_common *common)
{
	int			i;
	struct fsg_buffhd	*bh;
	enum fsg_state		old_state;
	struct fsg_lun		*curlun;
	unsigned int		exception_req_tag;

	/* Cancel all the pending transfers */
	if (common->fsg) {
		for (i = 0; i < FSG_NUM_BUFFERS; ++i) {
			bh = &common->buffhds[i];
			if (bh->inreq_busy)
				usb_ep_dequeue(common->fsg->bulk_in, bh->inreq);
			if (bh->outreq_busy)
				usb_ep_dequeue(common->fsg->bulk_out,
					       bh->outreq);
		}

		/* Wait until everything is idle */
		for (;;) {
			int num_active = 0;
			for (i = 0; i < FSG_NUM_BUFFERS; ++i) {
				bh = &common->buffhds[i];
				num_active += bh->inreq_busy + bh->outreq_busy;
			}
			if (num_active == 0)
				break;
			if (sleep_thread(common))
				return;
		}

		/* Clear out the controller's fifos */
		if (common->fsg->bulk_in_enabled)
			usb_ep_fifo_flush(common->fsg->bulk_in);
		if (common->fsg->bulk_out_enabled)
			usb_ep_fifo_flush(common->fsg->bulk_out);
	}

	/* Reset the I/O buffer states and pointers, the SCSI
	 * state, and the exception.  Then invoke the handler. */

	for (i = 0; i < FSG_NUM_BUFFERS; ++i) {
		bh = &common->buffhds[i];
		bh->state = BUF_STATE_EMPTY;
	}
	common->next_buffhd_to_fill = &common->buffhds[0];
	common->next_buffhd_to_drain = &common->buffhds[0];
	exception_req_tag = common->exception_req_tag;
	old_state = common->state;

	report_exception("handling", old_state);

	if (old_state == FSG_STATE_ABORT_BULK_OUT)
		common->state = FSG_STATE_STATUS_PHASE;
	else {
		for (i = 0; i < common->nluns; ++i) {
			curlun = &common->luns[i];
			curlun->sense_data = SS_NO_SENSE;
			curlun->info_valid = 0;
		}
		common->state = FSG_STATE_IDLE;
	}

	/* Carry out any extra actions required for the exception */
	switch (old_state) {
	case FSG_STATE_ABORT_BULK_OUT:
		send_status(common);

		if (common->state == FSG_STATE_STATUS_PHASE)
			common->state = FSG_STATE_IDLE;
		break;

	case FSG_STATE_RESET:
		/* In case we were forced against our will to halt a
		 * bulk endpoint, clear the halt now.  (The SuperH UDC
		 * requires this.) */
		if (!fsg_is_set(common))
			break;
		if (test_and_clear_bit(IGNORE_BULK_OUT,
				       &common->fsg->atomic_bitflags))
			usb_ep_clear_halt(common->fsg->bulk_in);

		if (common->ep0_req_tag == exception_req_tag)
			ep0_queue(common);	/* Complete the status stage */

		break;

	case FSG_STATE_CONFIG_CHANGE:
		do_set_interface(common, common->new_fsg);
		break;

	case FSG_STATE_EXIT:
	case FSG_STATE_TERMINATED:
		do_set_interface(common, NULL);		/* Free resources */
		common->state = FSG_STATE_TERMINATED;	/* Stop the thread */
		break;

	case FSG_STATE_INTERFACE_CHANGE:
	case FSG_STATE_DISCONNECT:
	case FSG_STATE_COMMAND_PHASE:
	case FSG_STATE_DATA_PHASE:
	case FSG_STATE_STATUS_PHASE:
	case FSG_STATE_IDLE:
		break;
	}
}

/*-------------------------------------------------------------------------*/

static void fsg_main_thread(void *fsg_)
{
	struct fsg_dev *fsg = fsg_dev_get(fsg_);
	struct fsg_common *common = fsg->common;
	struct f_ums_opts *opts = f_ums_opts_get(common->opts);
	struct fsg_buffhd *bh;
	unsigned i;
	int ret = 0;


	/* The main loop */
	while (common->state != FSG_STATE_TERMINATED) {
		if (exception_in_progress(common)) {
			handle_exception(common);
			continue;
		}

		if (!common->running) {
			ret = sleep_thread(common);
			if (ret)
				break;
			continue;
		}

		ret = get_next_command(common);
		if (ret)
			continue;

		if (!exception_in_progress(common))
			common->state = FSG_STATE_DATA_PHASE;

		if (do_scsi_command(common) || finish_reply(common))
			continue;

		if (!exception_in_progress(common))
			common->state = FSG_STATE_STATUS_PHASE;

		if (send_status(common))
			continue;

		if (!exception_in_progress(common))
			common->state = FSG_STATE_IDLE;
	}

	if (ret && ret != -ERESTARTSYS)
		pr_warn("%s: error %pe\n", __func__, ERR_PTR(ret));

	usb_free_all_descriptors(&fsg->function);

	for (i = 0; i < ums_count; i++)
		close(ums[i].fd);

	bh = common->buffhds;
	i = FSG_NUM_BUFFERS;

	do {
		dma_free(bh->buf);
	} while (++bh, --i);

	ums_count = 0;
	ums_files = NULL;

	f_ums_opts_put(opts);
	fsg_dev_put(fsg);
}

static void fsg_common_release(struct fsg_common *common);

static struct fsg_common *fsg_common_setup(struct f_ums_opts *opts)
{
	struct fsg_common *common;

	/* Allocate? */
	common = calloc(sizeof(*common), 1);
	if (!common)
		return NULL;

	common->ops = NULL;
	common->private_data = NULL;
	common->opts = opts;

	return common;
}

static int fsg_common_init(struct fsg_common *common,
			   struct usb_composite_dev *cdev)
{
	struct usb_gadget *gadget = cdev->gadget;
	struct file_list_entry *fentry;
	struct fsg_buffhd *bh;
	int nluns, i, rc;

	ums_count = 0;

	common->gadget = gadget;
	common->ep0 = gadget->ep0;
	common->ep0req = cdev->req;

	thread_task = bthread_run(fsg_main_thread, common->fsg, "mass-storage-gadget");
	if (IS_ERR(thread_task))
		return PTR_ERR(thread_task);

	file_list_detect_all(ums_files);

	file_list_for_each_entry(ums_files, fentry) {
		unsigned flags = O_RDWR;
		struct stat st;
		int fd;

		if (fentry->flags & ~FILE_LIST_FLAG_OPTIONAL)
			pr_warn("some flags will be ignored\n");

		fd = open(fentry->filename, flags);
		if (fd < 0) {
			if (fentry->flags & FILE_LIST_FLAG_OPTIONAL) {
				pr_info("skipping unavailable optional partition %s\n",
					fentry->filename);
				continue;
			}

			pr_err("open('%s') failed: %pe\n",
			       fentry->filename, ERR_PTR(fd));
			rc = fd;
			goto close;
		}

		rc = fstat(fd, &st);
		if (rc < 0) {
			pr_err("stat('%s') failed: %pe\n",
			       fentry->filename, ERR_PTR(rc));
			goto close;
		}

		if (st.st_size % SECTOR_SIZE != 0) {
			pr_err("exporting '%s' failed: invalid block size\n",
			       fentry->filename);
			goto close;
		}

		ums[ums_count].fd = fd;
		ums[ums_count].num_sectors = st.st_size / SECTOR_SIZE;

		strlcpy(ums[ums_count].name, fentry->name, sizeof(ums[ums_count].name));

		DBG(common, "LUN %d, %s sector_count %#x\n",
			ums_count, fentry->name, ums[ums_count].num_sectors);

		ums_count++;
	}

	/* Find out how many LUNs there should be */
	nluns = ums_count;
	if (nluns < 1 || nluns > FSG_MAX_LUNS) {
		pr_warn("invalid number of LUNs: %u\n", nluns);
		rc = -EINVAL;
		goto close;
	}

	/* Maybe allocate device-global string IDs, and patch descriptors */
	if (fsg_strings[FSG_STRING_INTERFACE].id == 0) {
		rc = usb_string_id(cdev);
		if (unlikely(rc < 0))
			goto error_release;
		fsg_strings[FSG_STRING_INTERFACE].id = rc;
		fsg_intf_desc.iInterface = rc;
	}

	common->nluns = nluns;

	for (i = 0; i < nluns; i++) {
		common->luns[i].removable = 1;

		rc = fsg_lun_open(&common->luns[i], ums[i].num_sectors, "");
		if (rc)
			goto error_luns;
	}
	common->lun = 0;

	/* Data buffers cyclic list */
	bh = common->buffhds;

	i = FSG_NUM_BUFFERS;
	goto buffhds_first_it;
	do {
		bh->next = bh + 1;
		++bh;
buffhds_first_it:
		bh->inreq_busy = 0;
		bh->outreq_busy = 0;
		bh->buf = dma_alloc(FSG_BUFLEN);
		if (unlikely(!bh->buf)) {
			rc = -ENOMEM;
			goto error_release;
		}
	} while (--i);
	bh->next = common->buffhds;

	snprintf(common->inquiry_string, sizeof common->inquiry_string,
		 "%-8s%-16s%04x",
		 "Linux   ",
		 "File-Store Gadget",
		 0xffff);

	/* Some peripheral controllers are known not to be able to
	 * halt bulk endpoints correctly.  If one of them is present,
	 * disable stalls.
	 */

	/* Information */
	DBG(common, FSG_DRIVER_DESC ", version: " FSG_DRIVER_VERSION "\n");
	DBG(common, "Number of LUNs=%d\n", common->nluns);

	return 0;

error_luns:
	common->nluns = i + 1;
error_release:
	common->state = FSG_STATE_TERMINATED;	/* The thread is dead */
	fsg_common_release(common);
close:
	for (i = 0; i < ums_count; i++)
		close(ums[i].fd);
	return rc;
}

static void fsg_common_release(struct fsg_common *common)
{
	/* If the thread isn't already dead, tell it to exit now */
	if (common->state != FSG_STATE_TERMINATED) {
		raise_exception(common, FSG_STATE_EXIT);
	}

	bthread_cancel(thread_task);
}


static void fsg_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct fsg_dev		*fsg = fsg_from_func(f);

	DBG(fsg, "unbind\n");

	if (fsg->common->fsg == fsg) {
		fsg->common->new_fsg = NULL;
		raise_exception(fsg->common, FSG_STATE_CONFIG_CHANGE);
	}

	fsg_common_release(fsg->common);
}

static int fsg_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct fsg_dev		*fsg = fsg_from_func(f);
	struct usb_gadget	*gadget = c->cdev->gadget;
	int			ret;
	struct usb_ep		*ep;
	unsigned		max_burst;
	struct fsg_common	*common = fsg->common;

	if (!ums_files) {
		struct f_ums_opts *opts = container_of(f->fi, struct f_ums_opts, func_inst);

		ums_files = opts->files;
	}

	fsg->gadget = gadget;

	DBG(fsg, "bind\n");

	ret = fsg_common_init(common, c->cdev);
	if (ret)
		goto remove_ums_files;

	/* New interface */
	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto fsg_common_release;
	fsg_intf_desc.bInterfaceNumber = ret;
	fsg->interface_number = ret;

	ret = -EOPNOTSUPP;

	/* Find all the endpoints we will use */
	ep = usb_ep_autoconfig(gadget, &fsg_fs_bulk_in_desc);
	if (!ep)
		goto autoconf_fail;

	ep->driver_data = common;	/* claim the endpoint */
	fsg->bulk_in = ep;

	ep = usb_ep_autoconfig(gadget, &fsg_fs_bulk_out_desc);
	if (!ep)
		goto autoconf_fail;

	ep->driver_data = common;	/* claim the endpoint */
	fsg->bulk_out = ep;

	/* Assume endpoint addresses are the same for both speeds */
	fsg_hs_bulk_in_desc.bEndpointAddress =
		fsg_fs_bulk_in_desc.bEndpointAddress;
	fsg_hs_bulk_out_desc.bEndpointAddress =
		fsg_fs_bulk_out_desc.bEndpointAddress;

	/* Calculate bMaxBurst, we know packet size is 1024 */
	max_burst = min_t(unsigned, FSG_BUFLEN / 1024, 15);

	fsg_ss_bulk_in_desc.bEndpointAddress =
		fsg_fs_bulk_in_desc.bEndpointAddress;
	fsg_ss_bulk_in_comp_desc.bMaxBurst = max_burst;

	fsg_ss_bulk_out_desc.bEndpointAddress =
		fsg_fs_bulk_out_desc.bEndpointAddress;
	fsg_ss_bulk_out_comp_desc.bMaxBurst = max_burst;

	/* Copy descriptors */
	return usb_assign_descriptors(f, fsg_fs_function, fsg_hs_function,
				      fsg_ss_function, fsg_ss_function);

autoconf_fail:
	ERROR(fsg, "unable to autoconfigure all endpoints\n");
fsg_common_release:
	fsg_common_release(common);
remove_ums_files:
	ums_files = NULL;

	return ret;
}


/****************************** ADD FUNCTION ******************************/

static struct usb_gadget_strings *fsg_strings_array[] = {
	&fsg_stringtab,
	NULL,
};

static void fsg_free(struct usb_function *f)
{
	struct fsg_dev *fsg;

	fsg = container_of(f, struct fsg_dev, function);

	fsg_dev_put(fsg);
}

static struct usb_function *fsg_alloc(struct usb_function_instance *fi)
{
	struct f_ums_opts *opts = fsg_opts_from_func_inst(fi);
	struct fsg_common *common = opts->common;
	struct fsg_dev *fsg;

	fsg = kzalloc(sizeof(*fsg), GFP_KERNEL);
	if (!fsg)
		return ERR_PTR(-ENOMEM);

	fsg->function.name = FSG_DRIVER_DESC;
	fsg->function.strings = fsg_strings_array;
	/* descriptors are per-instance copies */
	fsg->function.bind = fsg_bind;
	fsg->function.set_alt = fsg_set_alt;
	fsg->function.setup = fsg_setup;
	fsg->function.disable = fsg_disable;
	fsg->function.unbind = fsg_unbind;
	fsg->function.free_func = fsg_free;

	fsg->common = common;
	common->fsg = fsg_dev_get(fsg);

	return &fsg->function;
}

static void fsg_free_instance(struct usb_function_instance *fi)
{
	struct f_ums_opts *opts = fsg_opts_from_func_inst(fi);

	f_ums_opts_put(opts);
}

static struct usb_function_instance *fsg_alloc_inst(void)
{
	struct f_ums_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	opts->func_inst.free_func_inst = fsg_free_instance;

	opts->common = fsg_common_setup(opts);
	if (!opts->common) {
		free(opts);
		return ERR_PTR(-ENOMEM);
	}

	f_ums_opts_get(opts);

	return &opts->func_inst;
}

DECLARE_USB_FUNCTION_INIT(ums, fsg_alloc_inst, fsg_alloc);
