// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Most of this source has been derived from the Linux and
 * U-Boot USB Mass Storage driver implementations.
 *
 * Adapted for barebox:
 * Copyright (c) 2011, AMK Drives & Controls Ltd.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <dma.h>
#include <errno.h>
#include <scsi.h>
#include <linux/usb/usb.h>
#include <linux/usb/usb_defs.h>

#include <asm/unaligned.h>

#include "usb.h"
#include "transport.h"


/***********************************************************************
 * USB Storage routines
 ***********************************************************************/

#define USB_STOR_NO_REQUEST_SENSE	-1

static int usb_stor_request_sense(struct us_blk_dev *usb_blkdev)
{
	struct us_data *us = usb_blkdev->us;
	struct device *dev = &us->pusb_dev->dev;
	u8 cmd[6];
	const u8 datalen = 18;
	u8 *data = dma_alloc(datalen);

	dev_dbg(dev, "SCSI_REQ_SENSE\n");

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = SCSI_REQ_SENSE;
	cmd[4] = datalen;
	us->transport(usb_blkdev, cmd, sizeof(cmd), data, datalen);
	dev_dbg(dev, "Request Sense returned %02X %02X %02X\n",
		data[2], data[12], data[13]);

	dma_free(data);

	return 0;
}

static const char *usb_stor_opcode_name(u8 opcode)
{
	switch (opcode) {
	case SCSI_INQUIRY:
		return "SCSI_INQUIRY";
	case SCSI_TST_U_RDY:
		return "SCSI_TST_U_RDY";
	case SCSI_RD_CAPAC:
		return "SCSI_RD_CAPAC";
	case SCSI_READ10:
		return "SCSI_READ10";
	case SCSI_WRITE10:
		return "SCSI_WRITE10";
	};

	return "UNKNOWN";
}

static int usb_stor_transport(struct us_blk_dev *usb_blkdev,
			      const u8 *cmd, u8 cmdlen,
			      void *data, u32 datalen,
			      int retries, int request_sense_delay_ms)
{
	struct us_data *us = usb_blkdev->us;
	struct device *dev = &us->pusb_dev->dev;
	int i, ret;

	for (i = 0; i <= retries; i++) {
		dev_dbg(dev, "%s\n", usb_stor_opcode_name(cmd[0]));
		ret = us->transport(usb_blkdev, cmd, cmdlen, data, datalen);
		dev_dbg(dev, "%s returns %d\n", usb_stor_opcode_name(cmd[0]),
			ret);
		if (ret == USB_STOR_TRANSPORT_GOOD)
			return 0;

		if (request_sense_delay_ms == USB_STOR_NO_REQUEST_SENSE)
			continue;

		usb_stor_request_sense(usb_blkdev);

		if (request_sense_delay_ms)
			mdelay(request_sense_delay_ms);
	}

	dev_dbg(dev, "Retried %s %d times, and failed.\n", usb_stor_opcode_name(cmd[0]), retries);

	return -EIO;
}

static int usb_stor_inquiry(struct us_blk_dev *usb_blkdev)
{
	struct device *dev = &usb_blkdev->us->pusb_dev->dev;
	int ret;
	u8 cmd[6];
	const u16 datalen = 36;
	u8 *data = dma_alloc(datalen);

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = SCSI_INQUIRY;
	put_unaligned_be16(datalen, &cmd[3]);

	ret = usb_stor_transport(usb_blkdev, cmd, sizeof(cmd), data, datalen,
				 3, USB_STOR_NO_REQUEST_SENSE);
	if (ret < 0) {
		ret = -ENODEV;
		goto exit;
	}

	dev_dbg(dev, "Peripheral type: %x, removable: %x\n",
		data[0], (data[1] >> 7));
	dev_dbg(dev, "ISO ver: %x, resp format: %x\n",
		data[2], data[3]);
	dev_dbg(dev, "Vendor/product/rev: %28s\n",
		&data[8]);
	// TODO:  process and store device info

exit:
	dma_free(data);
	return ret;
}

static int usb_stor_test_unit_ready(struct us_blk_dev *usb_blkdev, u64 timeout_ns)
{
	u64 start;
	u8 cmd[6];
	int ret;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = SCSI_TST_U_RDY;

	start = get_time_ns();

	do {
		ret = usb_stor_transport(usb_blkdev, cmd, sizeof(cmd), NULL, 0,
					 10, 100);
	} while (ret < 0 && !is_timeout(start, timeout_ns));

	return ret ? -ENODEV : 0;
}

static int read_capacity_16(struct us_blk_dev *usb_blkdev)
{
	struct device *dev = &usb_blkdev->us->pusb_dev->dev;
	unsigned char cmd[16];
	const u8 datalen = 32;
	u8 *data = dma_alloc(datalen);
	int ret;
	sector_t lba;
	unsigned sector_size;

	memset(cmd, 0, 16);
	cmd[0] = SERVICE_ACTION_IN_16;
	cmd[1] = SAI_READ_CAPACITY_16;
	cmd[13] = datalen;

	ret = usb_stor_transport(usb_blkdev, cmd, sizeof(cmd), data, datalen,
				 3, USB_STOR_NO_REQUEST_SENSE);

	if (ret < 0) {
		dev_warn(dev, "Read Capacity(16) failed\n");
		goto fail;
	}

	/* Note this is logical, not physical sector size */
	sector_size = be32_to_cpup((u32 *)&data[8]);
	lba = be64_to_cpup((u64 *)&data[0]);

	dev_dbg(dev, "LBA (16) = 0x%llx w/ sector size = %u\n",
		lba, sector_size);

	if ((data[12] & 1) == 1) {
		dev_warn(dev, "Protection unsupported\n");
		ret = -ENOTSUPP;
		goto fail;
	}

	usb_blkdev->blk.blockbits = SECTOR_SHIFT;
	usb_blkdev->blk.num_blocks = lba + 1;

	ret = sector_size;
fail:
	dma_free(data);
	return ret;
}

static int read_capacity_10(struct us_blk_dev *usb_blkdev)
{
	struct device *dev = &usb_blkdev->us->pusb_dev->dev;
	unsigned char cmd[16];
	const u32 datalen = 8;
	__be32 *data = dma_alloc(datalen);
	int ret;
	sector_t lba;
	unsigned sector_size;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = SCSI_RD_CAPAC;

	ret = usb_stor_transport(usb_blkdev, cmd, sizeof(cmd), data, datalen,
				 3, USB_STOR_NO_REQUEST_SENSE);

	if (ret < 0) {
		dev_warn(dev, "Read Capacity(10) failed\n");
		goto fail;
	}

	sector_size = be32_to_cpu(data[1]);
	lba = be32_to_cpu(data[0]);

	dev_dbg(dev, "LBA (10) = 0x%llx w/ sector size = %u\n",
		lba, sector_size);

	if (sector_size != SECTOR_SIZE)
		dev_warn(dev, "Support only %d bytes sectors\n", SECTOR_SIZE);

	usb_blkdev->blk.num_blocks = lba + 1;
	usb_blkdev->blk.blockbits = SECTOR_SHIFT;

	ret = SECTOR_SIZE;
fail:
	dma_free(data);
	return ret;
}

static int usb_stor_io_16(struct us_blk_dev *usb_blkdev, u8 opcode,
			  sector_t start, u8 *data, u16 blocks)
{
	u8 cmd[16];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = opcode;
	put_unaligned_be64(start, &cmd[2]);
	put_unaligned_be32(blocks, &cmd[10]);

	return usb_stor_transport(usb_blkdev, cmd, sizeof(cmd), data,
				  blocks * SECTOR_SIZE, 10, 0);
}

static int usb_stor_io_10(struct us_blk_dev *usb_blkdev, u8 opcode,
			  sector_t start, u8 *data, u16 blocks)
{
	u8 cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = opcode;
	put_unaligned_be32(start, &cmd[2]);
	put_unaligned_be16(blocks, &cmd[7]);

	return usb_stor_transport(usb_blkdev, cmd, sizeof(cmd), data,
				  blocks * SECTOR_SIZE, 10, 0);
}

/***********************************************************************
 * Disk driver interface
 ***********************************************************************/

#define US_MAX_IO_BLK 32

/* Read / write a chunk of sectors on media */
static int usb_stor_blk_io(struct block_device *disk_dev,
			   sector_t sector_start, blkcnt_t sector_count, void *buffer,
			   bool read)
{
	struct us_blk_dev *pblk_dev = container_of(disk_dev,
						   struct us_blk_dev,
						   blk);
	struct us_data *us = pblk_dev->us;
	struct device *dev = &us->pusb_dev->dev;
	int result;

	/* ensure unit ready */
	dev_dbg(dev, "Testing for unit ready\n");
	if (usb_stor_test_unit_ready(pblk_dev, 0)) {
		dev_dbg(dev, "Device NOT ready\n");
		return -EIO;
	}

	/* read / write the requested data */
	dev_dbg(dev, "%s %llu block(s), starting from %llu\n",
		read ? "Read" : "Write",
		sector_count, sector_start);

	while (sector_count > 0) {
		u16 n = min_t(blkcnt_t, sector_count, US_MAX_IO_BLK);

		if (disk_dev->num_blocks > 0xffffffff) {
			result = usb_stor_io_16(pblk_dev,
						read ? SCSI_READ16 : SCSI_WRITE16,
						sector_start,
						buffer, n);
		} else {

			result = usb_stor_io_10(pblk_dev,
						read ? SCSI_READ10 : SCSI_WRITE10,
						sector_start,
						buffer, n);
		}

		if (result) {
			dev_dbg(dev, "I/O error at sector %llu\n", sector_start);
			break;
		}

		sector_start += n;
		sector_count -= n;
		buffer += n * SECTOR_SIZE;
	}

	return sector_count ? -EIO : 0;
}

/* Write a chunk of sectors to media */
static int __maybe_unused usb_stor_blk_write(struct block_device *blk,
				const void *buffer, sector_t block, blkcnt_t num_blocks)
{
	return usb_stor_blk_io(blk, block, num_blocks, (void *)buffer, false);
}

/* Read a chunk of sectors from media */
static int usb_stor_blk_read(struct block_device *blk, void *buffer, sector_t block,
				blkcnt_t num_blocks)
{
	return usb_stor_blk_io(blk, block, num_blocks, buffer, true);
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

/* Prepare a disk device */
static int usb_stor_init_blkdev(struct us_blk_dev *pblk_dev)
{
	struct us_data *us = pblk_dev->us;
	struct device *dev = &us->pusb_dev->dev;
	int result;

	/* get device info */
	dev_dbg(dev, "Reading device info\n");

	result = usb_stor_inquiry(pblk_dev);
	if (result) {
		dev_dbg(dev, "Cannot read device info\n");
		return result;
	}

	/* ensure unit ready */
	dev_dbg(dev, "Testing for unit ready\n");

	/* retry a bit longer than usual as some HDDs take longer to spin up */
	result = usb_stor_test_unit_ready(pblk_dev, 10ULL * NSEC_PER_SEC);
	if (result) {
		dev_dbg(dev, "Device NOT ready\n");
		return result;
	}

	/* read capacity */
	dev_dbg(dev, "Reading capacity\n");

	result = read_capacity_10(pblk_dev);
	if (result < 0)
		return result;

	if (pblk_dev->blk.num_blocks > 0xffffffff) {
		result = read_capacity_16(pblk_dev);
		if (result < 0) {
			dev_notice(dev, "Using 0xffffffff as device size\n");
			pblk_dev->blk.num_blocks = 1 + (sector_t) 0xffffffff;
		}
	}

	dev_dbg(dev, "Capacity = 0x%llx, blockshift = 0x%x\n",
		 pblk_dev->blk.num_blocks, pblk_dev->blk.blockbits);

	return 0;
}

/* Create and register a disk device for the specified LUN */
static int usb_stor_add_blkdev(struct us_data *us, unsigned char lun)
{
	struct device *dev = &us->pusb_dev->dev;
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
	dev_info(dev, "registering as disk%d\n", result);

	pblk_dev->blk.cdev.name = basprintf("disk%d", result);
	pblk_dev->blk.blockbits = SECTOR_SHIFT;
	pblk_dev->blk.type = BLK_TYPE_USB;

	result = blockdevice_register(&pblk_dev->blk);
	if (result != 0) {
		dev_err(dev, "Failed to register blockdevice\n");
		goto BadDevice;
	}

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
	struct device *dev = &us->pusb_dev->dev;
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
	struct device *dev = &us->pusb_dev->dev;
	unsigned int i;
	struct usb_endpoint_descriptor *ep;
	struct usb_endpoint_descriptor *ep_in = NULL;
	struct usb_endpoint_descriptor *ep_out = NULL;

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
	}
	if (!ep_in || !ep_out || us->protocol == US_PR_CBI) {
		dev_dbg(dev, "Endpoint sanity check failed! Rejecting dev.\n");
		return -EIO;
	}

	/* Store the pipe values */
	us->send_bulk_ep = USB_EP_NUM(ep_out);
	us->recv_bulk_ep = USB_EP_NUM(ep_in);

	return 0;
}

/* Scan device's LUNs, registering a disk device for each LUN */
static int usb_stor_scan(struct usb_device *usbdev, struct us_data *us)
{
	struct device *dev = &usbdev->dev;
	unsigned char lun;
	int num_devs = 0;

	/* obtain the max LUN */
	us->max_lun = 0;
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
	struct device *dev = &usbdev->dev;
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
	us->ifnum = intf->desc.bInterfaceNumber;
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

