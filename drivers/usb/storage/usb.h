/*
 * Most of this source has been derived from the Linux and
 * U-Boot USB Mass Storage driver implementations.
 *
 * Adapted for barebox:
 * Copyright (c) 2011, AMK Drives & Controls Ltd.
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
 */

#ifndef _STORAGE_USB_H_
#define _STORAGE_USB_H_

#include <usb/usb.h>
#include <block.h>
#include <disks.h>
#include <scsi.h>
#include <linux/list.h>


#ifdef  USB_STOR_DEBUG
#define US_DEBUGP(fmt, args...)    printf(fmt , ##args)
#else
#define US_DEBUGP(fmt, args...)
#endif


/* some defines, similar to ch9.h */
#define USB_EP_NUM(epd) \
	((epd)->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)
#define USB_EP_IS_DIR_IN(epd) \
	(((epd)->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
#define USB_EP_IS_XFER_BULK(epd) \
	(((epd)->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == \
	 USB_ENDPOINT_XFER_BULK)
#define USB_EP_IS_XFER_INT(epd) \
	(((epd)->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == \
	 USB_ENDPOINT_XFER_INT)
#define USB_EP_IS_INT_IN(epd) \
	(USB_EP_IS_XFER_INT(epd) && USB_EP_IS_DIR_IN(epd))


struct us_data;

typedef int (*trans_cmnd)(ccb *cb, struct us_data *data);
typedef int (*trans_reset)(struct us_data *data);

/* one us_data object allocated per usb storage device */
struct us_data {
	struct usb_device	*pusb_dev;	/* this usb_device */
	unsigned int		flags;		/* from filter */
	unsigned char		send_bulk_ep;	/* used endpoints */
	unsigned char		recv_bulk_ep;
	unsigned char		recv_intr_ep;
	unsigned char		ifnum;		/* interface number */

	unsigned char		subclass;
	unsigned char		protocol;

	unsigned char		max_lun;
	unsigned char		ep_bInterval;

	char			*transport_name;

	trans_cmnd		transport;	/* transport function */
	trans_reset		transport_reset;/* transport device reset */

	/* SCSI interfaces */
	ccb			*srb;		/* current srb */
	struct list_head	blk_dev_list;
};

/* one us_blk_dev object allocated per LUN */
struct us_blk_dev {
	struct us_data		*us;		/* LUN's enclosing dev */
	struct block_device	blk;		/* the blockdevice for the dev */
	unsigned char 		lun;		/* the LUN of this blk dev */
	struct list_head	list;		/* siblings */
};

#endif
