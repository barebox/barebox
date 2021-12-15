// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * storage_common.c -- Common definitions for mass storage functionality
 *
 * Copyright (C) 2003-2008 Alan Stern
 * Copyeight (C) 2009 Samsung Electronics
 * Author: Michal Nazarewicz (m.nazarewicz@samsung.com)
 *
 * Ported to u-boot:
 * Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *
 * Code refactoring & cleanup:
 * Łukasz Majewski <l.majewski@samsung.com>
 */

#include "storage_common.h"

/*
 * This file requires the following identifiers used in USB strings to
 * be defined (each of type pointer to char):
 *  - fsg_string_manufacturer -- name of the manufacturer
 *  - fsg_string_product      -- name of the product
 *  - fsg_string_serial       -- product's serial
 *  - fsg_string_config       -- name of the configuration
 *  - fsg_string_interface    -- name of the interface
 * The first four are only needed when FSG_DESCRIPTORS_DEVICE_STRINGS
 * macro is defined prior to including this file.
 */

/* There is only one interface. */

struct usb_interface_descriptor fsg_intf_desc = {
	.bLength =		sizeof fsg_intf_desc,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	2,		/* Adjusted during fsg_bind() */
	.bInterfaceClass =	USB_CLASS_MASS_STORAGE,
	.bInterfaceSubClass =	USB_SC_SCSI,	/* Adjusted during fsg_bind() */
	.bInterfaceProtocol =	USB_PR_BULK,	/* Adjusted during fsg_bind() */
	.iInterface =		FSG_STRING_INTERFACE,
};

/*
 * Three full-speed endpoint descriptors: bulk-in, bulk-out, and
 * interrupt-in.
 */

struct usb_endpoint_descriptor fsg_fs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	/* wMaxPacketSize set by autoconfiguration */
};

struct usb_endpoint_descriptor fsg_fs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	/* wMaxPacketSize set by autoconfiguration */
};

struct usb_descriptor_header *fsg_fs_function[] = {
	(struct usb_descriptor_header *) &fsg_intf_desc,
	(struct usb_descriptor_header *) &fsg_fs_bulk_in_desc,
	(struct usb_descriptor_header *) &fsg_fs_bulk_out_desc,
	NULL,
};

/*
 * USB 2.0 devices need to expose both high speed and full speed
 * descriptors, unless they only run at full speed.
 *
 * That means alternate endpoint descriptors (bigger packets)
 * and a "device qualifier" ... plus more construction options
 * for the configuration descriptor.
 */
struct usb_endpoint_descriptor fsg_hs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	/* bEndpointAddress copied from fs_bulk_in_desc during fsg_bind() */
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

struct usb_endpoint_descriptor fsg_hs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	/* bEndpointAddress copied from fs_bulk_out_desc during fsg_bind() */
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
	.bInterval =		1,	/* NAK every 1 uframe */
};

struct usb_descriptor_header *fsg_hs_function[] = {
	(struct usb_descriptor_header *) &fsg_intf_desc,
	(struct usb_descriptor_header *) &fsg_hs_bulk_in_desc,
	(struct usb_descriptor_header *) &fsg_hs_bulk_out_desc,
	NULL,
};

/* Maxpacket and other transfer characteristics vary by speed. */
struct usb_endpoint_descriptor *
fsg_ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *fs,
		struct usb_endpoint_descriptor *hs)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}

/*-------------------------------------------------------------------------*/

/*
 * If the next two routines are called while the gadget is registered,
 * the caller must own fsg->filesem for writing.
 */

int fsg_lun_open(struct fsg_lun *curlun, unsigned int num_sectors,
		 const char *filename)
{
	int				ro;

	/* R/W if we can, R/O if we must */
	ro = curlun->initially_ro;

	curlun->ro = ro;
	curlun->file_length = num_sectors << 9;
	curlun->num_sectors = num_sectors;
	debug("open backing file: %s\n", filename);

	return 0;
}

void fsg_lun_close(struct fsg_lun *curlun)
{
}

/*-------------------------------------------------------------------------*/

/*
 * Sync the file data, don't bother with the metadata.
 * This code was copied from fs/buffer.c:sys_fdatasync().
 */
int fsg_lun_fsync_sub(struct fsg_lun *curlun)
{
	return 0;
}

void store_cdrom_address(u8 *dest, int msf, u32 addr)
{
	if (msf) {
		/* Convert to Minutes-Seconds-Frames */
		addr >>= 2;		/* Convert to 2048-byte frames */
		addr += 2*75;		/* Lead-in occupies 2 seconds */
		dest[3] = addr % 75;	/* Frames */
		addr /= 75;
		dest[2] = addr % 60;	/* Seconds */
		addr /= 60;
		dest[1] = addr;		/* Minutes */
		dest[0] = 0;		/* Reserved */
	} else {
		/* Absolute sector */
		put_unaligned_be32(addr, dest);
	}
}

/*-------------------------------------------------------------------------*/
