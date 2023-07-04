// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Most of this source has been derived from the Linux and
 * U-Boot USB Mass Storage driver implementations.
 *
 * Adapted for barebox:
 * Copyright (c) 2011, AMK Drives & Controls Ltd.
 */

#include <common.h>
#include <clock.h>
#include <scsi.h>
#include <errno.h>
#include <dma.h>

#include "usb.h"
#include "transport.h"


/* The timeout argument of usb_bulk_msg() is actually ignored
   and the timeout is hardcoded in the host driver */
#define USB_BULK_TO		5000

static __u32 cbw_tag = 0;

/* direction table -- this indicates the direction of the data
 * transfer for each command code (bit-encoded) -- 1 indicates input
 * note that this doesn't work for shared command codes
 */
static const unsigned char us_direction[256/8] = {
	0x28, 0x81, 0x14, 0x14, 0x20, 0x01, 0x90, 0x77,
	0x0C, 0x20, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x40, 0x09, 0x01, 0x80, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#define US_DIRECTION(x) ((us_direction[x>>3] >> (x & 7)) & 1)


/*
 * Bulk only transport
 */

/* Clear a stall on an endpoint - special for bulk-only devices */
static int usb_stor_Bulk_clear_endpt_stall(struct us_data *us, unsigned int pipe)
{
	return usb_clear_halt(us->pusb_dev, pipe);
}

/* Determine what the maximum LUN supported is */
int usb_stor_Bulk_max_lun(struct us_data *us)
{
	struct device *dev = &us->pusb_dev->dev;
	int len, ret = 0;
	unsigned char *iobuf = dma_alloc(1);

	/* issue the command */
	iobuf[0] = 0;
	len = usb_control_msg(us->pusb_dev,
	                      usb_rcvctrlpipe(us->pusb_dev, 0),
	                      US_BULK_GET_MAX_LUN,
	                      USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
	                      0, us->ifnum, iobuf, 1, USB_CNTL_TIMEOUT);

	dev_dbg(dev, "GetMaxLUN command result is %d, data is %d\n",
		len, (int)iobuf[0]);

	/* if we have a successful request, return the result */
	if (len > 0)
		ret = iobuf[0];

	dma_free(iobuf);

	/*
	 * Some devices don't like GetMaxLUN.  They may STALL the control
	 * pipe, they may return a zero-length result, they may do nothing at
	 * all and timeout, or they may fail in even more bizarrely creative
	 * ways.  In these cases the best approach is to use the default
	 * value: only one LUN.
	 */
	return ret;
}

int usb_stor_Bulk_transport(struct us_blk_dev *usb_blkdev,
			    const u8 *cmd, u8 cmdlen,
			    void *data, u32 datalen)
{
	struct us_data *us = usb_blkdev->us;
	struct device *dev = &us->pusb_dev->dev;
	struct bulk_cb_wrap *cbw;
	struct bulk_cs_wrap *csw;
	int actlen, data_actlen;
	int result;
	unsigned int residue;
	unsigned int pipein = usb_rcvbulkpipe(us->pusb_dev, us->recv_bulk_ep);
	unsigned int pipeout = usb_sndbulkpipe(us->pusb_dev, us->send_bulk_ep);
	int dir_in = US_DIRECTION(cmd[0]);
	int ret = 0;

	cbw = dma_alloc(sizeof(*cbw));
	csw = dma_alloc(sizeof(*csw));

	/* set up the command wrapper */
	cbw->Signature = cpu_to_le32(US_BULK_CB_SIGN);
	cbw->DataTransferLength = cpu_to_le32(datalen);
	cbw->Flags = (dir_in ? US_BULK_FLAG_IN : US_BULK_FLAG_OUT);
	cbw->Tag = ++cbw_tag;
	cbw->Lun = usb_blkdev->lun;
	cbw->Length = cmdlen;

	/* copy the command payload */
	memset(cbw->CDB, 0, sizeof(cbw->CDB));
	memcpy(cbw->CDB, cmd, cbw->Length);

	/* send it to out endpoint */
	dev_dbg(dev, "Bulk Command S 0x%x T 0x%x L %d F %d Trg %d LUN %d CL %d\n",
		le32_to_cpu(cbw->Signature), cbw->Tag,
		le32_to_cpu(cbw->DataTransferLength), cbw->Flags,
		(cbw->Lun >> 4), (cbw->Lun & 0x0F),
		cbw->Length);
	result = usb_bulk_msg(us->pusb_dev, pipeout, cbw, US_BULK_CB_WRAP_LEN,
			      &actlen, USB_BULK_TO);
	dev_dbg(dev, "Bulk command transfer result=%d\n", result);
	if (result < 0) {
		usb_stor_Bulk_reset(us);
		ret = USB_STOR_TRANSPORT_FAILED;
		goto fail;
	}

	/* DATA STAGE */
	/* send/receive data payload, if there is any */

	mdelay(1);

	data_actlen = 0;
	if (datalen) {
		unsigned int pipe = dir_in ? pipein : pipeout;
		result = usb_bulk_msg(us->pusb_dev, pipe, data,
		                      datalen, &data_actlen, USB_BULK_TO);
		dev_dbg(dev, "Bulk data transfer result 0x%x\n", result);
		/* special handling of STALL in DATA phase */
		if ((result < 0) && (us->pusb_dev->status & USB_ST_STALLED)) {
			dev_dbg(dev, "DATA: stall\n");
			/* clear the STALL on the endpoint */
			result = usb_stor_Bulk_clear_endpt_stall(us, pipe);
		}
		if (result < 0) {
			dev_dbg(dev, "Device status: %lx\n", us->pusb_dev->status);
			usb_stor_Bulk_reset(us);
			ret = USB_STOR_TRANSPORT_FAILED;
			goto fail;
		}
	}

	/* STATUS phase + error handling */
	dev_dbg(dev, "Attempting to get CSW...\n");
	result = usb_bulk_msg(us->pusb_dev, pipein, csw, US_BULK_CS_WRAP_LEN,
	                      &actlen, USB_BULK_TO);

	/* did the endpoint stall? */
	if ((result < 0) && (us->pusb_dev->status & USB_ST_STALLED)) {
		dev_dbg(dev, "STATUS: stall\n");
		/* clear the STALL on the endpoint */
		result = usb_stor_Bulk_clear_endpt_stall(us, pipein);
		if (result >= 0) {
			dev_dbg(dev, "Attempting to get CSW...\n");
			result = usb_bulk_msg(us->pusb_dev, pipein,
			                      csw, US_BULK_CS_WRAP_LEN,
			                      &actlen, USB_BULK_TO);
		}
	}

	if (result < 0) {
		dev_dbg(dev, "Device status: %lx\n", us->pusb_dev->status);
		usb_stor_Bulk_reset(us);
		ret = USB_STOR_TRANSPORT_FAILED;
		goto fail;
	}

	/* check bulk status */
	residue = le32_to_cpu(csw->Residue);
	dev_dbg(dev, "Bulk Status S 0x%x T 0x%x R %u Stat 0x%x\n",
		le32_to_cpu(csw->Signature), csw->Tag, residue, csw->Status);
	if (csw->Signature != cpu_to_le32(US_BULK_CS_SIGN)) {
		dev_dbg(dev, "Bad CSW signature\n");
		usb_stor_Bulk_reset(us);
		ret = USB_STOR_TRANSPORT_FAILED;
	} else if (csw->Tag != cbw_tag) {
		dev_dbg(dev, "Mismatching tag\n");
		usb_stor_Bulk_reset(us);
		ret = USB_STOR_TRANSPORT_FAILED;
	} else if (csw->Status >= US_BULK_STAT_PHASE) {
		dev_dbg(dev, "Status >= phase\n");
		usb_stor_Bulk_reset(us);
		ret = USB_STOR_TRANSPORT_ERROR;
	} else if (residue > datalen) {
		dev_dbg(dev, "residue (%uB) > req data (%uB)\n",
		          residue, datalen);
		ret = USB_STOR_TRANSPORT_FAILED;
	} else if (csw->Status == US_BULK_STAT_FAIL) {
		dev_dbg(dev, "FAILED\n");
		ret = USB_STOR_TRANSPORT_FAILED;
	}

fail:
	dma_free(cbw);
	dma_free(csw);
	return ret;
}


/* This issues a Bulk-only Reset to the device in question, including
 * clearing the subsequent endpoint halts that may occur.
 */
int usb_stor_Bulk_reset(struct us_data *us)
{
	struct device *dev = &us->pusb_dev->dev;
	int result;
	int result2;
	unsigned int pipe;

	dev_dbg(dev, "%s called\n", __func__);

	/* issue the command */
	result = usb_control_msg(us->pusb_dev,
	                         usb_sndctrlpipe(us->pusb_dev, 0),
	                         US_BULK_RESET_REQUEST,
	                         USB_TYPE_CLASS | USB_RECIP_INTERFACE,
	                         0, us->ifnum, 0, 0, USB_CNTL_TIMEOUT);
	if ((result < 0) && (us->pusb_dev->status & USB_ST_STALLED)) {
		dev_dbg(dev, "Soft reset stalled: %d\n", result);
		return result;
	}
	mdelay(150);

	/* clear the bulk endpoints halt */
	dev_dbg(dev, "Soft reset: clearing %s endpoint halt\n", "bulk-in");
	pipe = usb_rcvbulkpipe(us->pusb_dev, us->recv_bulk_ep);
	result = usb_clear_halt(us->pusb_dev, pipe);
	mdelay(150);
	dev_dbg(dev, "Soft reset: clearing %s endpoint halt\n", "bulk-out");
	pipe = usb_sndbulkpipe(us->pusb_dev, us->send_bulk_ep);
	result2 = usb_clear_halt(us->pusb_dev, pipe);
	mdelay(150);

	if (result >= 0)
		result = result2;
	dev_dbg(dev, "Soft reset %s\n", ((result < 0) ? "failed" : "done"));

	return result;
}

