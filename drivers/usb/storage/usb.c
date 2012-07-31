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

#undef USB_STOR_DEBUG

#include "usb.h"
#include "transport.h"


static LIST_HEAD(us_blkdev_list);


/***********************************************************************
 * USB Storage routines
 ***********************************************************************/

static int usb_stor_inquiry(ccb *srb, struct us_data *us)
{
	int retries, result;

	srb->datalen = min(128UL, srb->datalen);
	if (srb->datalen < 5) {
		US_DEBUGP("SCSI_INQUIRY: invalid data buffer size\n");
		return -EINVAL;
	}

	retries = 3;
	do {
		US_DEBUGP("SCSI_INQUIRY\n");
		memset(&srb->cmd[0], 0, 6);
		srb->cmdlen = 6;
		srb->cmd[0] = SCSI_INQUIRY;
		srb->cmd[3] = (u8)(srb->datalen >> 8);
		srb->cmd[4] = (u8)(srb->datalen >> 0);
		result = us->transport(srb, us);
		US_DEBUGP("SCSI_INQUIRY returns %d\n", result);
	} while ((result != USB_STOR_TRANSPORT_GOOD) && retries--);

	return (result != USB_STOR_TRANSPORT_GOOD) ? -EIO : 0;
}

static int usb_stor_request_sense(ccb *srb, struct us_data *us)
{
	unsigned char *pdata = srb->pdata;
	unsigned long datalen = srb->datalen;

	US_DEBUGP("SCSI_REQ_SENSE\n");
	srb->pdata = &srb->sense_buf[0];
	srb->datalen = 18;
	memset(&srb->cmd[0], 0, 6);
	srb->cmdlen = 6;
	srb->cmd[0] = SCSI_REQ_SENSE;
	srb->cmd[4] = (u8)(srb->datalen >> 0);
	us->transport(srb, us);
	US_DEBUGP("Request Sense returned %02X %02X %02X\n",
	          srb->sense_buf[2], srb->sense_buf[12], srb->sense_buf[13]);
	srb->pdata = pdata;
	srb->datalen = datalen;

	return 0;
}

static int usb_stor_test_unit_ready(ccb *srb, struct us_data *us)
{
	int retries, result;

	retries = 10;
	do {
		US_DEBUGP("SCSI_TST_U_RDY\n");
		memset(&srb->cmd[0], 0, 12);
		srb->cmdlen = 12;
		srb->cmd[0] = SCSI_TST_U_RDY;
		srb->datalen = 0;
		result = us->transport(srb, us);
		US_DEBUGP("SCSI_TST_U_RDY returns %d\n", result);
		if (result == USB_STOR_TRANSPORT_GOOD)
			return 0;
		usb_stor_request_sense(srb, us);
		wait_ms(100);
	} while (retries--);

	return -1;
}

static int usb_stor_read_capacity(ccb *srb, struct us_data *us)
{
	int retries, result;

	if (srb->datalen < 8) {
		US_DEBUGP("SCSI_RD_CAPAC: invalid data buffer size\n");
		return -EINVAL;
	}

	retries = 3;
	do {
		US_DEBUGP("SCSI_RD_CAPAC\n");
		memset(&srb->cmd[0], 0, 10);
		srb->cmdlen = 10;
		srb->cmd[0] = SCSI_RD_CAPAC;
		srb->datalen = 8;
		result = us->transport(srb, us);
		US_DEBUGP("SCSI_RD_CAPAC returns %d\n", result);
	} while ((result != USB_STOR_TRANSPORT_GOOD) && retries--);

	return (result != USB_STOR_TRANSPORT_GOOD) ? -EIO : 0;
}

static int usb_stor_read_10(ccb *srb, struct us_data *us,
                            unsigned long start, unsigned short blocks)
{
	int retries, result;

	retries = 2;
	do {
		US_DEBUGP("SCSI_READ10: start %lx blocks %x\n", start, blocks);
		memset(&srb->cmd[0], 0, 10);
		srb->cmdlen = 10;
		srb->cmd[0] = SCSI_READ10;
		srb->cmd[2] = (u8)(start >> 24);
		srb->cmd[3] = (u8)(start >> 16);
		srb->cmd[4] = (u8)(start >> 8);
		srb->cmd[5] = (u8)(start >> 0);
		srb->cmd[7] = (u8)(blocks >> 8);
		srb->cmd[8] = (u8)(blocks >> 0);
		result = us->transport(srb, us);
		US_DEBUGP("SCSI_READ10 returns %d\n", result);
		if (result == USB_STOR_TRANSPORT_GOOD)
			return 0;
		usb_stor_request_sense(srb, us);
	} while (retries--);

	return -EIO;
}

static int usb_stor_write_10(ccb *srb, struct us_data *us,
                             unsigned long start, unsigned short blocks)
{
	int retries, result;

	retries = 2;
	do {
		US_DEBUGP("SCSI_WRITE10: start %lx blocks %x\n", start, blocks);
		memset(&srb->cmd[0], 0, 10);
		srb->cmdlen = 10;
		srb->cmd[0] = SCSI_WRITE10;
		srb->cmd[2] = (u8)(start >> 24);
		srb->cmd[3] = (u8)(start >> 16);
		srb->cmd[4] = (u8)(start >> 8);
		srb->cmd[5] = (u8)(start >> 0);
		srb->cmd[7] = (u8)(blocks >> 8);
		srb->cmd[8] = (u8)(blocks >> 0);
		result = us->transport(srb, us);
		US_DEBUGP("SCSI_WRITE10 returns %d\n", result);
		if (result == USB_STOR_TRANSPORT_GOOD)
			return 0;
		usb_stor_request_sense(srb, us);
	} while (retries--);

	return us->transport(srb, us);
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
	ccb us_ccb;
	unsigned sectors_done;

	if (sector_count == 0)
		return 0;

	/* check for unsupported block size */
	if (pblk_dev->blk.blockbits != SECTOR_SHIFT) {
		US_DEBUGP("%s: unsupported block shift %d\n",
		          __func__, pblk_dev->blk.blockbits);
		return -EINVAL;
	}

	/* check for invalid sector_start */
	if (sector_start >= pblk_dev->blk.num_blocks || sector_start > (ulong)-1) {
		US_DEBUGP("%s: start sector %d too large\n",
		          __func__, sector_start);
		return -EINVAL;
	}

	us_ccb.lun = pblk_dev->lun;
	usb_disable_asynch(1);

	/* ensure unit ready */
	US_DEBUGP("Testing for unit ready\n");
	if (usb_stor_test_unit_ready(&us_ccb, us)) {
		US_DEBUGP("Device NOT ready\n");
		usb_disable_asynch(0);
		return -EIO;
	}

	/* possibly limit the amount of I/O data */
	if (sector_count > INT_MAX) {
		sector_count = INT_MAX;
		US_DEBUGP("Restricting I/O to %u blocks\n", sector_count);
	}
	if (sector_start + sector_count > pblk_dev->blk.num_blocks) {
		sector_count = pblk_dev->blk.num_blocks - sector_start;
		US_DEBUGP("Restricting I/O to %u blocks\n", sector_count);
	}

	/* read / write the requested data */
	US_DEBUGP("%s %u block(s), starting from %d\n",
	          ((io_op == io_rd) ? "Read" : "Write"),
	          sector_count, sector_start);
	sectors_done = 0;
	while (sector_count > 0) {
		int result;
		unsigned n = min(sector_count, US_MAX_IO_BLK);
		us_ccb.pdata = buffer + (sectors_done * SECTOR_SIZE);
		us_ccb.datalen = n * SECTOR_SIZE;
		if (io_op == io_rd)
			result = usb_stor_read_10(&us_ccb, us,
			                          (ulong)sector_start, n);
		else
			result = usb_stor_write_10(&us_ccb, us,
			                           (ulong)sector_start, n);
		if (result != 0) {
			US_DEBUGP("I/O error at sector %d\n", sector_start);
			break;
		}
		sector_start += n;
		sector_count -= n;
		sectors_done += n;
	}

	usb_disable_asynch(0);

	US_DEBUGP("Successful I/O of %d blocks\n", sectors_done);

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

static unsigned char us_io_buf[512];

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
	ccb us_ccb;
	unsigned long *pcap;
	int result = 0;

	us_ccb.pdata = us_io_buf;
	us_ccb.lun = pblk_dev->lun;

	pblk_dev->blk.num_blocks = 0;
	usb_disable_asynch(1);

	/* get device info */
	US_DEBUGP("Reading device info\n");
	us_ccb.datalen = 36;
	if (usb_stor_inquiry(&us_ccb, us)) {
		US_DEBUGP("Cannot read device info\n");
		result = -ENODEV;
		goto Exit;
	}
	US_DEBUGP("Peripheral type: %x, removable: %x\n",
	          us_io_buf[0], (us_io_buf[1] >> 7));
	US_DEBUGP("ISO ver: %x, resp format: %x\n", us_io_buf[2], us_io_buf[3]);
	US_DEBUGP("Vendor/product/rev: %28s\n", &us_io_buf[8]);
	// TODO:  process and store device info

	/* ensure unit ready */
	US_DEBUGP("Testing for unit ready\n");
	us_ccb.datalen = 0;
	if (usb_stor_test_unit_ready(&us_ccb, us)) {
		US_DEBUGP("Device NOT ready\n");
		result = -ENODEV;
		goto Exit;
	}

	/* read capacity */
	US_DEBUGP("Reading capacity\n");
	memset(us_ccb.pdata, 0, 8);
	us_ccb.datalen = sizeof(us_io_buf);
	if (usb_stor_read_capacity(&us_ccb, us) != 0) {
		US_DEBUGP("Cannot read device capacity\n");
		result = -EIO;
		goto Exit;
	}
	pcap = (unsigned long *)us_ccb.pdata;
	US_DEBUGP("Read Capacity returns: 0x%lx, 0x%lx\n", pcap[0], pcap[1]);
	pblk_dev->blk.num_blocks = usb_limit_blk_cnt(be32_to_cpu(pcap[0]) + 1);
	if (be32_to_cpu(pcap[1]) != SECTOR_SIZE)
		pr_warn("Support only %d bytes sectors\n", SECTOR_SIZE);
	pblk_dev->blk.blockbits = SECTOR_SHIFT;
	US_DEBUGP("Capacity = 0x%x, blockshift = 0x%x\n",
	          pblk_dev->blk.num_blocks, pblk_dev->blk.blockbits);

Exit:
	usb_disable_asynch(0);
	return result;
}

/* Create and register a disk device for the specified LUN */
static int usb_stor_add_blkdev(struct us_data *us, struct device_d *dev,
							unsigned char lun)
{
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

	pblk_dev->blk.cdev.name = asprintf("disk%d", result);
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
	US_DEBUGP("USB disk device successfully added\n");

	return 0;

BadDevice:
	US_DEBUGP("%s failed with %d\n", __func__, result);
	free(pblk_dev);
	return result;
}

/***********************************************************************
 * USB Mass Storage device probe and initialization
 ***********************************************************************/

/* Get the transport settings */
static void get_transport(struct us_data *us)
{
	switch (us->protocol) {
	case US_PR_BULK:
		us->transport_name = "Bulk";
		us->transport = &usb_stor_Bulk_transport;
		us->transport_reset = &usb_stor_Bulk_reset;
		break;
	}

	US_DEBUGP("Transport: %s\n", us->transport_name);
}

/* Get the endpoint settings */
static int get_pipes(struct us_data *us, struct usb_interface_descriptor *intf)
{
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
	for (i = 0; i < intf->bNumEndpoints; i++) {
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
		US_DEBUGP("Endpoint sanity check failed! Rejecting dev.\n");
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
	unsigned char lun;
	int num_devs = 0;

	/* obtain the max LUN */
	us->max_lun = 0;
	if (us->protocol == US_PR_BULK)
		us->max_lun = usb_stor_Bulk_max_lun(us);

	/* register a disk device for each active LUN */
	for (lun=0; lun<=us->max_lun; lun++) {
		if (usb_stor_add_blkdev(us, &usbdev->dev, lun) == 0)
			num_devs++;
	}

	US_DEBUGP("Found %d block devices on %s\n", num_devs, usbdev->dev.name);

	return  num_devs ? 0 : -ENODEV;
}

/* Probe routine for standard devices */
static int usb_stor_probe(struct usb_device *usbdev,
			 const struct usb_device_id *id)
{
	struct us_data *us;
	int result;
	int ifno;
	struct usb_interface_descriptor *intf;

	US_DEBUGP("Supported USB Mass Storage device detected\n");

	/* scan usbdev interfaces again to find one that we can handle */
	for (ifno=0; ifno<usbdev->config.no_of_if; ifno++) {
		intf = &usbdev->config.if_desc[ifno];

		if (intf->bInterfaceClass    == USB_CLASS_MASS_STORAGE &&
		    intf->bInterfaceSubClass == US_SC_SCSI &&
		    intf->bInterfaceProtocol == US_PR_BULK)
			break;
	}
	if (ifno >= usbdev->config.no_of_if)
		return -ENXIO;

	/* select the right interface */
	result = usb_set_interface(usbdev, intf->bInterfaceNumber, 0);
	if (result)
		return result;

	US_DEBUGP("Selected interface %d\n", (int)intf->bInterfaceNumber);

	/* allocate us_data structure */
	us = (struct us_data *)malloc(sizeof(struct us_data));
	if (!us)
		return -ENOMEM;
	memset(us, 0, sizeof(struct us_data));

	/* initialize the us_data structure */
	us->pusb_dev = usbdev;
	us->flags = 0;
	us->ifnum = intf->bInterfaceNumber;
	us->subclass = intf->bInterfaceSubClass;
	us->protocol = intf->bInterfaceProtocol;
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
	US_DEBUGP("%s failed with %d\n", __func__, result);
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

