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

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <errno.h>
#include <scsi.h>
#include <usb/usb.h>
#include <usb/usb_defs.h>

#include "usb.h"
#include "transport.h"


static LIST_HEAD(us_blkdev_list);


/***********************************************************************
 * USB Storage routines
 ***********************************************************************/

static int usb_stor_inquiry(struct us_blk_dev *usb_blkdev)
{
	struct us_data *us = usb_blkdev->us;
	struct device_d *dev = &us->pusb_dev->dev;
	int retries, result;
	u8 cmd[6];
	const u32 datalen = 36;
	u8 *data = xzalloc(datalen);

	retries = 3;
	do {
		dev_dbg(dev, "SCSI_INQUIRY\n");
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = SCSI_INQUIRY;
		cmd[3] = (u8)(datalen >> 8);
		cmd[4] = (u8)(datalen >> 0);
		result = us->transport(usb_blkdev, cmd, sizeof(cmd),
				       data, datalen);
		dev_dbg(dev, "SCSI_INQUIRY returns %d\n", result);
		if (result == USB_STOR_TRANSPORT_GOOD) {
			dev_dbg(dev, "Peripheral type: %x, removable: %x\n",
				data[0], (data[1] >> 7));
			dev_dbg(dev, "ISO ver: %x, resp format: %x\n",
				data[2], data[3]);
			dev_dbg(dev, "Vendor/product/rev: %28s\n",
				&data[8]);
			// TODO:  process and store device info
			break;
		}
	} while (retries--);

	free(data);

	return result ? -ENODEV : 0;
}

static int usb_stor_request_sense(struct us_blk_dev *usb_blkdev)
{
	struct us_data *us = usb_blkdev->us;
	struct device_d *dev = &us->pusb_dev->dev;
	u8 cmd[6];
	const u8 datalen = 18;
	u8 *data = xzalloc(datalen);

	dev_dbg(dev, "SCSI_REQ_SENSE\n");

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = SCSI_REQ_SENSE;
	cmd[4] = datalen;
	us->transport(usb_blkdev, cmd, sizeof(cmd), data, datalen);
	dev_dbg(dev, "Request Sense returned %02X %02X %02X\n",
		data[2], data[12], data[13]);

	free(data);

	return 0;
}

static int usb_stor_test_unit_ready(struct us_blk_dev *usb_blkdev)
{
	struct us_data *us = usb_blkdev->us;
	struct device_d *dev = &us->pusb_dev->dev;
	u8 cmd[12];
	int retries, result;

	retries = 10;
	do {
		dev_dbg(dev, "SCSI_TST_U_RDY\n");
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = SCSI_TST_U_RDY;
		result = us->transport(usb_blkdev, cmd, sizeof(cmd),
				       NULL, 0);
		dev_dbg(dev, "SCSI_TST_U_RDY returns %d\n", result);
		if (result == USB_STOR_TRANSPORT_GOOD)
			return 0;
		usb_stor_request_sense(usb_blkdev);
		mdelay(100);
	} while (retries--);

	return -ENODEV;
}

static int usb_stor_read_capacity(struct us_blk_dev *usb_blkdev,
				  u32 *last_lba, u32 *block_length)
{
	struct us_data *us = usb_blkdev->us;
	struct device_d *dev = &us->pusb_dev->dev;
	int retries, result;
	const u32 datalen = 8;
	u32 *data = xzalloc(datalen);
	u8 cmd[10];

	retries = 3;
	do {
		dev_dbg(dev, "SCSI_RD_CAPAC\n");
		memset(cmd, 0, sizeof(cmd));
		cmd[0] = SCSI_RD_CAPAC;

		result = us->transport(usb_blkdev, cmd, sizeof(cmd), data,
				       datalen);
		dev_dbg(dev, "SCSI_RD_CAPAC returns %d\n", result);

		if (result == USB_STOR_TRANSPORT_GOOD) {
			dev_dbg(dev, "Read Capacity returns: 0x%x, 0x%x\n",
				data[0], data[1]);
			*last_lba = be32_to_cpu(data[0]);
			*block_length = be32_to_cpu(data[1]);
			break;
		}
	} while (retries--);

	free(data);

	return result ? -EIO : 0;
}

static int usb_stor_io_10(struct us_blk_dev *usb_blkdev, u8 opcode,
			  unsigned long start, u8 *data,
			  unsigned short blocks)
{
	struct us_data *us = usb_blkdev->us;
	int retries, result;
	const u32 datalen = blocks * SECTOR_SIZE;
	u8 cmd[10];

	retries = 2;
	do {
		memset(cmd, 0, sizeof(cmd));

		cmd[0] = opcode;
		cmd[2] = (u8)(start >> 24);
		cmd[3] = (u8)(start >> 16);
		cmd[4] = (u8)(start >> 8);
		cmd[5] = (u8)(start >> 0);
		cmd[7] = (u8)(blocks >> 8);
		cmd[8] = (u8)(blocks >> 0);
		result = us->transport(usb_blkdev, cmd, sizeof(cmd),
				       data, datalen);
		if (result == USB_STOR_TRANSPORT_GOOD)
			return 0;
		usb_stor_request_sense(usb_blkdev);
	} while (retries--);

	return -EIO;
}

/***********************************************************************
 * Disk driver interface
 ***********************************************************************/

#define US_MAX_IO_BLK 32

#define to_usb_mass_storage(x) container_of((x), struct us_blk_dev, blk)

enum { io_rd, io_wr };

/* Read / write a chunk of sectors on media */
static int usb_stor_blk_io(int io_op, struct block_device *disk_dev,
			int sector_start, int sector_count, void *buffer)
{
	struct us_blk_dev *pblk_dev = to_usb_mass_storage(disk_dev);
	struct us_data *us = pblk_dev->us;
	struct device_d *dev = &us->pusb_dev->dev;
	unsigned sectors_done;

	if (sector_count == 0)
		return 0;

	/* check for unsupported block size */
	if (pblk_dev->blk.blockbits != SECTOR_SHIFT) {
		dev_dbg(dev, "%s: unsupported block shift %d\n",
			 __func__, pblk_dev->blk.blockbits);
		return -EINVAL;
	}

	/* check for invalid sector_start */
	if (sector_start >= pblk_dev->blk.num_blocks || sector_start > (ulong)-1) {
		dev_dbg(dev, "%s: start sector %d too large\n",
			 __func__, sector_start);
		return -EINVAL;
	}

	usb_disable_asynch(1);

	/* ensure unit ready */
	dev_dbg(dev, "Testing for unit ready\n");
	if (usb_stor_test_unit_ready(pblk_dev)) {
		dev_dbg(dev, "Device NOT ready\n");
		usb_disable_asynch(0);
		return -EIO;
	}

	/* possibly limit the amount of I/O data */
	if (sector_count > INT_MAX) {
		sector_count = INT_MAX;
		dev_dbg(dev, "Restricting I/O to %u blocks\n", sector_count);
	}
	if (sector_start + sector_count > pblk_dev->blk.num_blocks) {
		sector_count = pblk_dev->blk.num_blocks - sector_start;
		dev_dbg(dev, "Restricting I/O to %u blocks\n", sector_count);
	}

	/* read / write the requested data */
	dev_dbg(dev, "%s %u block(s), starting from %d\n",
		 (io_op == io_rd) ? "Read" : "Write",
		 sector_count, sector_start);
	sectors_done = 0;
	while (sector_count > 0) {
		int result;
		unsigned n = min(sector_count, US_MAX_IO_BLK);
		u8 *data = buffer + (sectors_done * SECTOR_SIZE);

		result = usb_stor_io_10(pblk_dev,
					(io_op == io_rd) ?
					SCSI_READ10 : SCSI_WRITE10,
					(ulong)sector_start,
					data, n);
		if (result != 0) {
			dev_dbg(dev, "I/O error at sector %d\n", sector_start);
			break;
		}
		sector_start += n;
		sector_count -= n;
		sectors_done += n;
	}

	usb_disable_asynch(0);

	dev_dbg(dev, "Successful I/O of %d blocks\n", sectors_done);

	return (sector_count != 0) ? -EIO : 0;
}

/* Write a chunk of sectors to media */
static int __maybe_unused usb_stor_blk_write(struct block_device *blk,
				const void *buffer, int block, int num_blocks)
{
	return usb_stor_blk_io(io_wr, blk, block, num_blocks, (void *)buffer);
}

/* Read a chunk of sectors from media */
static int usb_stor_blk_read(struct block_device *blk, void *buffer, int block,
				int num_blocks)
{
	return usb_stor_blk_io(io_rd, blk, block, num_blocks, buffer);
}

static struct block_device_ops usb_mass_storage_ops = {
	.read = usb_stor_blk_read,
#ifdef CONFIG_BLOCK_WRITE
	.write = usb_stor_blk_write,
#endif
};

/***********************************************************************
 * Block device routines
 ***********************************************************************/

static int usb_limit_blk_cnt(unsigned cnt)
{
	if (cnt > 0x7fffffff) {
		pr_warn("Limiting device size due to 31 bit contraints\n");
		return 0x7fffffff;
	}

	return (int)cnt;
}

/* Prepare a disk device */
static int usb_stor_init_blkdev(struct us_blk_dev *pblk_dev)
{
	struct us_data *us = pblk_dev->us;
	struct device_d *dev = &us->pusb_dev->dev;
	u32 last_lba = 0, block_length = 0;
	int result = 0;

	pblk_dev->blk.num_blocks = 0;
	usb_disable_asynch(1);

	/* get device info */
	dev_dbg(dev, "Reading device info\n");

	result = usb_stor_inquiry(pblk_dev);
	if (result) {
		dev_dbg(dev, "Cannot read device info\n");
		goto Exit;
	}

	/* ensure unit ready */
	dev_dbg(dev, "Testing for unit ready\n");

	result = usb_stor_test_unit_ready(pblk_dev);
	if (result) {
		dev_dbg(dev, "Device NOT ready\n");
		goto Exit;
	}

	/* read capacity */
	dev_dbg(dev, "Reading capacity\n");

	result = usb_stor_read_capacity(pblk_dev, &last_lba, &block_length);
	if (result < 0) {
		dev_dbg(dev, "Cannot read device capacity\n");
		goto Exit;
	}

	pblk_dev->blk.num_blocks = usb_limit_blk_cnt(last_lba + 1);
	if (block_length != SECTOR_SIZE)
		pr_warn("Support only %d bytes sectors\n", SECTOR_SIZE);
	pblk_dev->blk.blockbits = SECTOR_SHIFT;
	dev_dbg(dev, "Capacity = 0x%x, blockshift = 0x%x\n",
		 pblk_dev->blk.num_blocks, pblk_dev->blk.blockbits);

Exit:
	usb_disable_asynch(0);
	return result;
}

/* Create and register a disk device for the specified LUN */
static int usb_stor_add_blkdev(struct us_data *us, unsigned char lun)
{
	struct device_d *dev = &us->pusb_dev->dev;
	struct us_blk_dev *pblk_dev;
	int result;

	/* allocate a new USB block device */
	pblk_dev = xzalloc(sizeof(struct us_blk_dev));

	/* initialize blk dev data */
	pblk_dev->blk.dev = dev;
	pblk_dev->blk.ops = &usb_mass_storage_ops;
	pblk_dev->us = us;
	pblk_dev->lun = lun;

	/* read some info and get the unit ready */
	result = usb_stor_init_blkdev(pblk_dev);
	if (result < 0)
		goto BadDevice;

	result = cdev_find_free_index("disk");
	if (result == -1)
		pr_err("Cannot find a free number for the disk node\n");
	pr_info("Using index %d for the new disk\n", result);

	pblk_dev->blk.cdev.name = basprintf("disk%d", result);
	pblk_dev->blk.blockbits = SECTOR_SHIFT;

	result = blockdevice_register(&pblk_dev->blk);
	if (result != 0) {
		dev_err(dev, "Failed to register blockdevice\n");
		goto BadDevice;
	}

	/* create partitions on demand */
	result = parse_partition_table(&pblk_dev->blk);
	if (result != 0)
		dev_warn(dev, "No partition table found\n");

	list_add_tail(&pblk_dev->list, &us->blk_dev_list);
	dev_dbg(dev, "USB disk device successfully added\n");

	return 0;

BadDevice:
	dev_dbg(dev, "%s failed with %d\n", __func__, result);
	free(pblk_dev);
	return result;
}

/***********************************************************************
 * USB Mass Storage device probe and initialization
 ***********************************************************************/

/* Get the transport settings */
static void get_transport(struct us_data *us)
{
	struct device_d *dev = &us->pusb_dev->dev;
	switch (us->protocol) {
	case US_PR_BULK:
		us->transport_name = "Bulk";
		us->transport = &usb_stor_Bulk_transport;
		us->transport_reset = &usb_stor_Bulk_reset;
		break;
	}

	dev_dbg(dev, "Transport: %s\n", us->transport_name);
}

/* Get the endpoint settings */
static int get_pipes(struct us_data *us, struct usb_interface *intf)
{
	struct device_d *dev = &us->pusb_dev->dev;
	unsigned int i;
	struct usb_endpoint_descriptor *ep;
	struct usb_endpoint_descriptor *ep_in = NULL;
	struct usb_endpoint_descriptor *ep_out = NULL;
	struct usb_endpoint_descriptor *ep_int = NULL;

	/*
	 * Find the first endpoint of each type we need.
	 * We are expecting a minimum of 2 endpoints - in and out (bulk).
	 * An optional interrupt-in is OK (necessary for CBI protocol).
	 * We will ignore any others.
	 */
	for (i = 0; i < intf->desc.bNumEndpoints; i++) {
		ep = &intf->ep_desc[i];

		if (USB_EP_IS_XFER_BULK(ep)) {
			if (USB_EP_IS_DIR_IN(ep)) {
				if ( !ep_in )
					ep_in = ep;
			}
			else {
				if ( !ep_out )
					ep_out = ep;
			}
		}
		else if (USB_EP_IS_INT_IN(ep)) {
			if (!ep_int)
				ep_int = ep;
		}
	}
	if (!ep_in || !ep_out || (us->protocol == US_PR_CBI && !ep_int)) {
		dev_dbg(dev, "Endpoint sanity check failed! Rejecting dev.\n");
		return -EIO;
	}

	/* Store the pipe values */
	us->send_bulk_ep = USB_EP_NUM(ep_out);
	us->recv_bulk_ep = USB_EP_NUM(ep_in);
	if (ep_int) {
		us->recv_intr_ep = USB_EP_NUM(ep_int);
		us->ep_bInterval = ep_int->bInterval;
	}
	return 0;
}

/* Scan device's LUNs, registering a disk device for each LUN */
static int usb_stor_scan(struct usb_device *usbdev, struct us_data *us)
{
	struct device_d *dev = &usbdev->dev;
	unsigned char lun;
	int num_devs = 0;

	/* obtain the max LUN */
	us->max_lun = 1;
	if (us->protocol == US_PR_BULK)
		us->max_lun = usb_stor_Bulk_max_lun(us);

	/* register a disk device for each active LUN */
	for (lun=0; lun<=us->max_lun; lun++) {
		if (usb_stor_add_blkdev(us, lun) == 0)
			num_devs++;
	}

	dev_dbg(dev, "Found %d block devices on %s\n", num_devs, usbdev->dev.name);

	return  num_devs ? 0 : -ENODEV;
}

/* Probe routine for standard devices */
static int usb_stor_probe(struct usb_device *usbdev,
			 const struct usb_device_id *id)
{
	struct device_d *dev = &usbdev->dev;
	struct us_data *us;
	int result;
	int ifno;
	struct usb_interface *intf;

	dev_dbg(dev, "Supported USB Mass Storage device detected\n");

	/* scan usbdev interfaces again to find one that we can handle */
	for (ifno=0; ifno<usbdev->config.no_of_if; ifno++) {
		intf = &usbdev->config.interface[ifno];

		if (intf->desc.bInterfaceClass    == USB_CLASS_MASS_STORAGE &&
		    intf->desc.bInterfaceSubClass == US_SC_SCSI &&
		    intf->desc.bInterfaceProtocol == US_PR_BULK)
			break;
	}
	if (ifno >= usbdev->config.no_of_if)
		return -ENXIO;

	/* select the right interface */
	result = usb_set_interface(usbdev, intf->desc.bInterfaceNumber, 0);
	if (result)
		return result;

	dev_dbg(dev, "Selected interface %d\n", (int)intf->desc.bInterfaceNumber);

	/* allocate us_data structure */
	us = xzalloc(sizeof(*us));

	/* initialize the us_data structure */
	us->pusb_dev = usbdev;
	us->flags = 0;
	us->ifnum = intf->desc.bInterfaceNumber;
	us->subclass = intf->desc.bInterfaceSubClass;
	us->protocol = intf->desc.bInterfaceProtocol;
	INIT_LIST_HEAD(&us->blk_dev_list);

	/* get standard transport and protocol settings */
	get_transport(us);

	/* find the endpoints needed by the transport */
	result = get_pipes(us, intf);
	if (result)
		goto BadDevice;

	/* register a disk device for each LUN */
	usb_stor_scan(usbdev, us);

	/* associate the us_data structure with the usb_device */
	usbdev->drv_data = us;

	return 0;

BadDevice:
	dev_dbg(dev, "%s failed with %d\n", __func__, result);
	free(us);
	return result;
}

/* Handle a USB mass-storage disconnect */
static void usb_stor_disconnect(struct usb_device *usbdev)
{
	struct us_data *us = (struct us_data *)usbdev->drv_data;
	struct us_blk_dev *bdev, *bdev_tmp;

	list_for_each_entry_safe(bdev, bdev_tmp, &us->blk_dev_list, list) {
		list_del(&bdev->list);
		blockdevice_unregister(&bdev->blk);
		free(bdev);
	}

	/* release device's private data */
	usbdev->drv_data = 0;
	free(us);
}

#define USUAL_DEV(use_proto, use_trans, drv_info) \
{	USB_INTERFACE_INFO(USB_CLASS_MASS_STORAGE, use_proto, use_trans), \
	.driver_info = (drv_info) }

/* Table with supported devices, most specific first. */
static struct usb_device_id usb_storage_usb_ids[] = {
	USUAL_DEV(US_SC_SCSI, US_PR_BULK, 0),	// SCSI intf, BBB proto
	{ }
};


/***********************************************************************
 * USB Storage driver initialization and registration
 ***********************************************************************/

static struct usb_driver usb_storage_driver = {
	.name =		"usb-storage",
	.id_table =	usb_storage_usb_ids,
	.probe =	usb_stor_probe,
	.disconnect =	usb_stor_disconnect,
};

static int __init usb_stor_init(void)
{
	return usb_driver_register(&usb_storage_driver);
}
device_initcall(usb_stor_init);

