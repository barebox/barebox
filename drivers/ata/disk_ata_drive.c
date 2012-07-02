/*
 * Copyright (C) 2011 Juergen Beisert, Pengutronix
 *
 * Inspired from various soures like http://wiki.osdev.org/ATA_PIO_Mode,
 * u-boot and the linux kernel
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
 */

#include <common.h>
#include <xfuncs.h>
#include <io.h>
#include <malloc.h>
#include <errno.h>
#include <clock.h>
#include <block.h>
#include <ata_drive.h>
#include <disks.h>

#define ATA_CMD_ID_DEVICE 0xEC
#define ATA_CMD_RD_CONF 0x40
#define ATA_CMD_RD	0x20
#define ATA_CMD_WR	0x30

#define DISK_MASTER 0
#define DISK_SLAVE 1

/* max timeout for a rotating disk in [ms] */
#define MAX_TIMEOUT 5000

/**
 * Collection of data we need to know about this drive
 */
struct ata_drive_access {
	struct block_device blk; /**< the main device */
	struct ata_ioports *io;	/**< register file */
	uint16_t id[(SECTOR_SIZE / sizeof(uint16_t))];
};

#define to_ata_drive_access(x) container_of((x), struct ata_drive_access, blk)

#define ata_id_u32(id,n)        \
        (((uint32_t) (id)[(n) + 1] << 16) | ((uint32_t) (id)[(n)]))
#define ata_id_u64(id,n)        \
        ( ((uint64_t) (id)[(n) + 3] << 48) | \
          ((uint64_t) (id)[(n) + 2] << 32) | \
          ((uint64_t) (id)[(n) + 1] << 16) | \
          ((uint64_t) (id)[(n) + 0]) )

#define ata_id_has_lba(id)               ((id)[49] & (1 << 9))

/* drive's status flags */
#define ATA_STATUS_BUSY (1 << 7)
#define ATA_STATUS_READY (1 << 6)
#define ATA_STATUS_WR_FLT (1 << 5)
#define ATA_STATUS_DRQ (1 << 4)
#define ATA_STATUS_CORR (1 << 3)
#define ATA_STATUS_ERROR (1 << 1)
/* command flags */
#define LBA_FLAG (1 << 6)
#define ATA_DEVCTL_SOFT_RESET (1 << 2)
#define ATA_DEVCTL_INTR_DISABLE (1 << 1)

enum {
	ATA_ID_SERNO		= 10,
#define ATA_ID_SERNO_LEN 20
	ATA_ID_FW_REV		= 23,
#define ATA_ID_FW_REV_LEN 8
	ATA_ID_PROD		= 27,
#define ATA_ID_PROD_LEN 40
	ATA_ID_CAPABILITY	= 49,
	ATA_ID_FIELD_VALID	= 53,
	ATA_ID_LBA_CAPACITY	= 60,
	ATA_ID_MWDMA_MODES	= 63,
	ATA_ID_PIO_MODES	= 64,
	ATA_ID_QUEUE_DEPTH	= 75,
	ATA_ID_MAJOR_VER	= 80,
	ATA_ID_COMMAND_SET_1	= 82,
	ATA_ID_COMMAND_SET_2	= 83,
	ATA_ID_CFSSE		= 84,
	ATA_ID_CFS_ENABLE_1	= 85,
	ATA_ID_CFS_ENABLE_2	= 86,
	ATA_ID_CSF_DEFAULT	= 87,
	ATA_ID_UDMA_MODES	= 88,
	ATA_ID_HW_CONFIG	= 93,
	ATA_ID_LBA_CAPACITY_2	= 100,
};

static int ata_id_is_valid(const uint16_t *id)
{
	if ((id[ATA_ID_FIELD_VALID] & 1) == 0) {
		pr_debug("Drive's ID seems invalid\n");
		return -EINVAL;
	}

	return 0;
}

static inline int ata_id_has_lba48(const uint16_t *id)
{
	if ((id[ATA_ID_COMMAND_SET_2] & 0xC000) != 0x4000)
		return 0;
	if (!ata_id_u64(id, ATA_ID_LBA_CAPACITY_2))
		return 0;
	return id[ATA_ID_COMMAND_SET_2] & (1 << 10);
}

static uint64_t ata_id_n_sectors(uint16_t *id)
{
	if (ata_id_has_lba(id)) {
		if (ata_id_has_lba48(id))
			return ata_id_u64(id, ATA_ID_LBA_CAPACITY_2);
		else
			return ata_id_u32(id, ATA_ID_LBA_CAPACITY);
	}

	return 0;
}

static void ata_id_string(const uint16_t *id, unsigned char *s,
				unsigned ofs, unsigned len)
{
	unsigned c;

	while (len > 0) {
		c = id[ofs] >> 8;
		*s = c;
		s++;

		c = id[ofs] & 0xff;
		*s = c;
		s++;

		ofs++;
		len -= 2;
	}
}

static void ata_id_c_string(const uint16_t *id, unsigned char *s,
				unsigned ofs, unsigned len)
{
	unsigned char *p;

	ata_id_string(id, s, ofs, len - 1);

	p = s + strnlen((char *)s, len - 1);
	while (p > s && p[-1] == ' ')
		p--;
	*p = '\0';
}

static void __maybe_unused ata_dump_id(uint16_t *id)
{
	unsigned char serial[ATA_ID_SERNO_LEN + 1];
	unsigned char firmware[ATA_ID_FW_REV_LEN + 1];
	unsigned char product[ATA_ID_PROD_LEN + 1];
	uint64_t n_sectors;

	/* Serial number */
	ata_id_c_string(id, serial, ATA_ID_SERNO, sizeof(serial));
	printf("S/N: %s\n\r", serial);

	/* Firmware version */
	ata_id_c_string(id, firmware, ATA_ID_FW_REV, sizeof(firmware));
	printf("Firmware version: %s\n\r", firmware);

	/* Product model */
	ata_id_c_string(id, product, ATA_ID_PROD, sizeof(product));
	printf("Product model number: %s\n\r", product);

	/* Total sectors of device  */
	n_sectors = ata_id_n_sectors(id);
	printf("Capablity: %lld sectors\n\r", n_sectors);

	printf ("id[49]: capabilities = 0x%04x\n"
		"id[53]: field valid = 0x%04x\n"
		"id[63]: mwdma = 0x%04x\n"
		"id[64]: pio = 0x%04x\n"
		"id[75]: queue depth = 0x%04x\n",
		id[ATA_ID_CAPABILITY],
		id[ATA_ID_FIELD_VALID],
		id[ATA_ID_MWDMA_MODES],
		id[ATA_ID_PIO_MODES],
		id[ATA_ID_QUEUE_DEPTH]);

	printf ("id[76]: sata capablity = 0x%04x\n"
		"id[78]: sata features supported = 0x%04x\n"
		"id[79]: sata features enable = 0x%04x\n",
		id[76], /* FIXME */
		id[78], /* FIXME */
		id[79]); /* FIXME */

	printf ("id[80]: major version = 0x%04x\n"
		"id[81]: minor version = 0x%04x\n"
		"id[82]: command set supported 1 = 0x%04x\n"
		"id[83]: command set supported 2 = 0x%04x\n"
		"id[84]: command set extension = 0x%04x\n",
		id[ATA_ID_MAJOR_VER],
		id[81], /* FIXME */
		id[ATA_ID_COMMAND_SET_1],
		id[ATA_ID_COMMAND_SET_2],
		id[ATA_ID_CFSSE]);
	printf ("id[85]: command set enable 1 = 0x%04x\n"
		"id[86]: command set enable 2 = 0x%04x\n"
		"id[87]: command set default = 0x%04x\n"
		"id[88]: udma = 0x%04x\n"
		"id[93]: hardware reset result = 0x%04x\n",
		id[ATA_ID_CFS_ENABLE_1],
		id[ATA_ID_CFS_ENABLE_2],
		id[ATA_ID_CSF_DEFAULT],
		id[ATA_ID_UDMA_MODES],
		id[ATA_ID_HW_CONFIG]);
}

/**
 * Swap little endian data on demand
 * @param buf Buffer with little endian word data
 * @param wds 16 bit word count
 *
 * ATA disks report their ID data in little endian notation on a 16 bit word
 * base. So swap the buffer content if the running CPU differs in their
 * endiaeness.
 */
static void ata_fix_endianess(uint16_t *buf, unsigned wds)
{
#ifdef __BIG_ENDIAN
	unsigned u;

	for (u = 0; u < wds; u++)
		buf[u] = le16_to_cpu(buf[u]);
#endif
}

/**
 * Read the status register of the ATA drive
 * @param io Register file
 * @return Register's content
 */
static uint8_t ata_rd_status(struct ata_ioports *io)
{
	return readb(io->status_addr);
}

/**
 * Wait until the disk is busy or time out
 * @param io Register file
 * @param timeout Timeout in [ms]
 * @return 0 on success, -ETIMEDOUT else
 */
static int ata_wait_busy(struct ata_ioports *io, unsigned timeout)
{
	uint8_t status;
	uint64_t start = get_time_ns();
	uint64_t toffs = timeout * 1000 * 1000;

	do {
		status = ata_rd_status(io);
		if (status & ATA_STATUS_BUSY)
			return 0;
	} while (!is_timeout(start, toffs));

	return -ETIMEDOUT;
}

/**
 * Wait until the disk is ready again or time out
 * @param io Register file
 * @param timeout Timeout in [ms]
 * @return 0 on success, -ETIMEDOUT else
 *
 * This function is useful to check if the disk has accepted a command.
 */
static int ata_wait_ready(struct ata_ioports *io, unsigned timeout)
{
	uint8_t status;
	uint64_t start = get_time_ns();
	uint64_t toffs = timeout * 1000 * 1000;

	do {
		status = ata_rd_status(io);
		if (!(status & ATA_STATUS_BUSY)) {
			if (status & ATA_STATUS_READY)
				return 0;
		}
	} while (!is_timeout(start, toffs));

	return -ETIMEDOUT;
}

/**
 * Setup the sector number in LBA notation (LBA28)
 * @param io Register file
 * @param drive 0 master drive, 1 slave drive
 * @param num Sector number
 *
 * @todo LBA48 support
 */
static int ata_set_lba_sector(struct ata_ioports *io, unsigned drive, uint64_t num)
{
	if (num > 0x0FFFFFFF || drive > 1)
		return -EINVAL;

	writeb(0xA0 | LBA_FLAG | drive << 4 | num >> 24, io->device_addr);
	writeb(0x00, io->error_addr);
	writeb(0x01, io->nsect_addr);
	writeb(num, io->lbal_addr);	/* 0 ... 7 */
	writeb(num >> 8, io->lbam_addr); /* 8 ... 15 */
	writeb(num >> 16, io->lbah_addr); /* 16 ... 23 */

	return 0;
}

/**
 * Write an ATA command into the disk
 * @param io Register file
 * @param cmd Command to write
 * @return 0 on success
 */
static int ata_wr_cmd(struct ata_ioports *io, uint8_t cmd)
{
	int rc;

	rc = ata_wait_ready(io, MAX_TIMEOUT);
	if (rc != 0)
		return rc;

	writeb(cmd, io->command_addr);
	return 0;
}

/**
 * Write a new value into the "device control register"
 * @param io Register file
 * @param val Value to write
 */
static void ata_wr_dev_ctrl(struct ata_ioports *io, uint8_t val)
{
	writeb(val, io->ctl_addr);
}

/**
 * Read one sector from the drive (always SECTOR_SIZE bytes at once)
 * @param io Register file
 * @param buf Buffer to read the data into
 */
static void ata_rd_sector(struct ata_ioports *io, void *buf)
{
	unsigned u = SECTOR_SIZE / sizeof(uint16_t);
	uint16_t *b = buf;

	if (io->dataif_be) {
		for (; u > 0; u--)
			*b++ = be16_to_cpu(readw(io->data_addr));
	} else {
		for (; u > 0; u--)
			*b++ = le16_to_cpu(readw(io->data_addr));
	}
}

/**
 * Write one sector into the drive
 * @param io Register file
 * @param buf Buffer to read the data from
 */
static void ata_wr_sector(struct ata_ioports *io, const void *buf)
{
	unsigned u = SECTOR_SIZE / sizeof(uint16_t);
	const uint16_t *b = buf;

	if (io->dataif_be) {
		for (; u > 0; u--)
			writew(cpu_to_be16(*b++), io->data_addr);
	} else {
		for (; u > 0; u--)
			writew(cpu_to_le16(*b++), io->data_addr);
	}
}

/**
 * Read the ATA disk's description info
 * @param d All we need to know about the disk
 * @return 0 on success
 */
static int ata_get_id(struct ata_drive_access *d)
{
	int rc;

	writeb(0xA0, d->io->device_addr);	/* FIXME drive */
	writeb(0x00, d->io->lbal_addr);
	writeb(0x00, d->io->lbam_addr);
	writeb(0x00, d->io->lbah_addr);

	rc = ata_wr_cmd(d->io, ATA_CMD_ID_DEVICE);
	if (rc != 0)
		return rc;

	rc = ata_wait_ready(d->io, MAX_TIMEOUT);
	if (rc != 0)
		return rc;

	ata_rd_sector(d->io, &d->id);

	ata_fix_endianess(d->id, SECTOR_SIZE / sizeof(uint16_t));

	return ata_id_is_valid(d->id);
}

static int ata_reset(struct ata_ioports *io)
{
	int rc;
	uint8_t reg;

	/* try a hard reset first (if available) */
	if (io->reset != NULL) {
		pr_debug("%s: Resetting drive...\n", __func__);
		io->reset(1);
		rc = ata_wait_busy(io, 500);
		io->reset(0);
		if (rc == 0) {
			rc = ata_wait_ready(io, MAX_TIMEOUT);
			if (rc != 0)
				return rc;
		} else {
			pr_debug("%s: Drive does not respond to RESET line. Ignored\n",
					__func__);
		}
	}

	/* try a soft reset */
	ata_wr_dev_ctrl(io, ATA_DEVCTL_SOFT_RESET | ATA_DEVCTL_INTR_DISABLE);
	rc = ata_wait_busy(io, MAX_TIMEOUT);	/* does the drive accept the command? */
	if (rc != 0) {
		pr_debug("%s: Drive fails on soft reset\n", __func__);
		return rc;
	}
	ata_wr_dev_ctrl(io, ATA_DEVCTL_INTR_DISABLE);
	rc = ata_wait_ready(io, MAX_TIMEOUT);
	if (rc != 0) {
		pr_debug("%s: Drive fails after soft reset\n", __func__);
		return rc;
	}

	reg = ata_rd_status(io) & 0xf;

	if (reg == 0xf) {
		pr_debug("%s: Seems no drive connected!\n", __func__);
		return -ENODEV;
	}

	return 0;
}

/**
 * Read a chunk of sectors from the drive
 * @param blk All info about the block device we need
 * @param buffer Buffer to read into
 * @param block Sector's LBA number to start read from
 * @param num_blocks Sector count to read
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to store all data!
 *
 * @note Due to 'block' is of type 'int' only small disks can be handled!
 * @todo Optimize the read loop
 */
static int ata_read(struct block_device *blk, void *buffer, int block,
				int num_blocks)
{
	int rc;
	uint64_t sector = block;
	struct ata_drive_access *drv = to_ata_drive_access(blk);

	while (num_blocks) {
		rc = ata_set_lba_sector(drv->io, DISK_MASTER, sector);
		if (rc != 0)
			return rc;
		rc = ata_wr_cmd(drv->io, ATA_CMD_RD);
		if (rc != 0)
			return rc;
		rc = ata_wait_ready(drv->io, MAX_TIMEOUT);
		if (rc != 0)
			return rc;
		ata_rd_sector(drv->io, buffer);
		num_blocks--;
		sector++;
		buffer += SECTOR_SIZE;
	}

	return 0;
}

/**
 * Write a chunk of sectors into the drive
 * @param blk All info about the block device we need
 * @param buffer Buffer to write from
 * @param block Sector's number to start write to
 * @param num_blocks Sector count to write
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to read all data!
 *
 * @note Due to 'block' is of type 'int' only small disks can be handled!
 * @todo Optimize the write loop
 */
static int __maybe_unused ata_write(struct block_device *blk,
				const void *buffer, int block, int num_blocks)
{
	int rc;
	uint64_t sector = block;
	struct ata_drive_access *drv = to_ata_drive_access(blk);

	while (num_blocks) {
		rc = ata_set_lba_sector(drv->io, DISK_MASTER, sector);
		if (rc != 0)
			return rc;
		rc = ata_wr_cmd(drv->io, ATA_CMD_WR);
		if (rc != 0)
			return rc;
		ata_wr_sector(drv->io, buffer);
		num_blocks--;
		sector++;
		buffer += SECTOR_SIZE;
	}

	return 0;
}

static struct block_device_ops ata_ops = {
	.read = ata_read,
#ifdef CONFIG_BLOCK_WRITE
	.write = ata_write,
#endif
};

/* until Barebox can handle 64 bit offsets */
static int limit_disk_size(uint64_t val)
{
	if (val > (__INT_MAX__ / SECTOR_SIZE))
		return (__INT_MAX__ / SECTOR_SIZE);
	return (int)val;
}

/**
 * Register an ATA drive behind an IDE like interface
 * @param dev The interface device
 * @param io ATA register file description
 * @return 0 on success
 */
int register_ata_drive(struct device_d *dev, struct ata_ioports *io)
{
	int rc;
	struct ata_drive_access *drive;

	drive = xzalloc(sizeof(struct ata_drive_access));

	drive->io = io;
	drive->blk.dev = dev;
	drive->blk.ops = &ata_ops;

	rc = ata_reset(io);
	if (rc) {
		dev_dbg(dev, "Resetting failed\n");
		goto on_error;
	}

	rc = ata_get_id(drive);
	if (rc != 0) {
		dev_dbg(dev, "Reading ID failed\n");
		goto on_error;
	}

#ifdef DEBUG
	ata_dump_id(drive->id);
#endif
	rc = cdev_find_free_index("disk");
	if (rc == -1)
		pr_err("Cannot find a free index for the disk node\n");

	drive->blk.num_blocks = limit_disk_size(ata_id_n_sectors(drive->id));
	drive->blk.cdev.name = asprintf("disk%d", rc);
	drive->blk.blockbits = SECTOR_SHIFT;

	rc = blockdevice_register(&drive->blk);
	if (rc != 0) {
		dev_err(dev, "Failed to register blockdevice\n");
		goto on_error;
	}

	/* create partitions on demand */
	rc = parse_partition_table(&drive->blk);
	if (rc != 0)
		dev_warn(dev, "No partition table found\n");

	return 0;

on_error:
	free(drive);
	return rc;
}

/**
 * @file
 * @brief Generic ATA disk drive support
 *
 * Please be aware: This driver covers only a subset of the available ATA drives
 *
 * @todo Support for disks larger than 4 GiB
 * @todo LBA48
 * @todo CHS
 */
