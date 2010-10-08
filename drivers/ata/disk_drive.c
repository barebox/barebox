/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/**
 * @file
 * @brief Generic disk drive support
 *
 * @todo Support for disks larger than 4 GiB
 * @todo Reliable size detection for BIOS based disks (on x86 only)
 */

#include <stdio.h>
#include <init.h>
#include <driver.h>
#include <types.h>
#include <ata.h>
#include <xfuncs.h>
#include <errno.h>
#include <string.h>
#include <linux/kernel.h>
#include <malloc.h>

/**
 * Description of one partition table entry (D*S type)
 */
struct partition_entry {
	uint8_t boot_indicator;
	uint8_t chs_begin[3];
	uint8_t type;
	uint8_t chs_end[3];
	uint32_t partition_start;
	uint32_t partition_size;
} __attribute__ ((packed));

/** one for all */
#define SECTOR_SIZE 512

/**
 * Guess the size of the disk, based on the partition table entries
 * @param dev device to create partitions for
 * @param table partition table
 * @return size in sectors
 */
static unsigned long disk_guess_size(struct device_d *dev, struct partition_entry *table)
{
	int part_order[4] = {0, 1, 2, 3};
	unsigned long size = 0;
	int i;

	/* TODO order the partitions */

	for (i = 0; i < 4; i++) {
		if (table[part_order[i]].partition_start != 0) {
			size += table[part_order[i]].partition_start - size; /* the gap */
			size += table[part_order[i]].partition_size;
		}
	}
#if 1
/* limit disk sizes we can't handle due to 32 bit limits */
	if (size > 0x7fffff) {
		dev_warn(dev, "Warning: Size limited due to 32 bit contraints\n");
		size = 0x7fffff;
	}
#endif
	return size;
}

/**
 * Register partitions found on the drive
 * @param dev device to create partitions for
 * @param table partition table
 * @return 0 on success
 */
static int disk_register_partitions(struct device_d *dev, struct partition_entry *table)
{
	int part_order[4] = {0, 1, 2, 3};
	int i, rc;
	char drive_name[16], partition_name[19];

	/* TODO order the partitions */

	for (i = 0; i < 4; i++) {
		sprintf(drive_name, "%s%d", dev->name, dev->id);
		sprintf(partition_name, "%s%d.%d", dev->name, dev->id, i);
		if (table[part_order[i]].partition_start != 0) {
#if 1
/* ignore partitions we can't handle due to 32 bit limits */
			if (table[part_order[i]].partition_start > 0x7fffff)
				continue;
			if (table[part_order[i]].partition_size > 0x7fffff)
				continue;
#endif
			dev_dbg(dev, "Registering partition %s to drive %s\n",
				partition_name, drive_name);
			rc = devfs_add_partition(drive_name,
				table[part_order[i]].partition_start * SECTOR_SIZE,
				table[part_order[i]].partition_size * SECTOR_SIZE,
				DEVFS_PARTITION_FIXED, partition_name);
			if (rc != 0)
				dev_err(dev, "Failed to register partition %s (%d)\n", partition_name, rc);
		}
	}

	return 0;
}

/**
 * Write some data to a disk
 * @param cdev the device to write to
 * @param _buf source of data
 * @param count byte count to write
 * @param offset where to write to disk
 * @param flags Ignored
 * @return Written bytes or negative value in case of failure
 */
static ssize_t disk_write(struct cdev *cdev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	struct device_d *dev = cdev->dev;
	struct ata_interface *intf = dev->platform_data;
	int rc;
	unsigned sep_count = offset & (SECTOR_SIZE - 1);
	ssize_t written = 0;

	/* starting at no sector boundary? */
	if (sep_count != 0) {
		uint8_t tmp_buf[SECTOR_SIZE];
		unsigned to_write = min(SECTOR_SIZE - sep_count, count);

		rc = intf->read(dev, offset / SECTOR_SIZE, 1, tmp_buf);
		if (rc != 0) {
			dev_err(dev, "Cannot read data\n");
			return -1;
		}
		memcpy(&tmp_buf[sep_count], _buf, to_write);
		rc = intf->write(dev, offset / SECTOR_SIZE, 1, tmp_buf);
		if (rc != 0) {
			dev_err(dev, "Cannot write data\n");
			return -1;
		}

		_buf += to_write;
		offset += to_write;
		count -= to_write;
		written += to_write;
	}

	/* full sector part */
	sep_count = count / SECTOR_SIZE;
	if (sep_count) {
		rc = intf->write(dev, offset / SECTOR_SIZE, sep_count, _buf);
		if (rc != 0) {
			dev_err(dev, "Cannot write data\n");
			return -1;
		}
		_buf += sep_count * SECTOR_SIZE;
		offset += sep_count * SECTOR_SIZE;
		count -= sep_count * SECTOR_SIZE;
		written += sep_count * SECTOR_SIZE;
	}

	/* ending at no sector boundary? */
	if (count) {
		uint8_t tmp_buf[SECTOR_SIZE];

		rc = intf->read(dev, offset / SECTOR_SIZE, 1, tmp_buf);
		if (rc != 0) {
			dev_err(dev, "Cannot read data\n");
			return -1;
		}
		memcpy(tmp_buf, _buf, count);
		rc = intf->write(dev, offset / SECTOR_SIZE, 1, tmp_buf);
		if (rc != 0) {
			dev_err(dev, "Cannot write data\n");
			return -1;
		}
		written += count;
	}

	return written;
}

/**
 * Read some data from a disk
 * @param cdev the device to read from
 * @param _buf destination of the data
 * @param count byte count to read
 * @param offset where to read from
 * @param flags Ignored
 * @return Read bytes or negative value in case of failure
 */
static ssize_t disk_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	struct device_d *dev = cdev->dev;
	struct ata_interface *intf = dev->platform_data;
	int rc;
	unsigned sep_count = offset & (SECTOR_SIZE - 1);
	ssize_t read = 0;

	/* starting at no sector boundary? */
	if (sep_count != 0) {
		uint8_t tmp_buf[SECTOR_SIZE];
		unsigned to_read = min(SECTOR_SIZE - sep_count, count);

		rc = intf->read(dev, offset / SECTOR_SIZE, 1, tmp_buf);
		if (rc != 0) {
			dev_err(dev, "Cannot read data\n");
			return -1;
		}
		memcpy(_buf, &tmp_buf[sep_count], to_read);
		_buf += to_read;
		offset += to_read;
		count -= to_read;
		read += to_read;
	}

	/* full sector part */
	sep_count = count / SECTOR_SIZE;
	if (sep_count) {
		rc = intf->read(dev, offset / SECTOR_SIZE, sep_count, _buf);
		if (rc != 0) {
			dev_err(dev, "Cannot read data\n");
			return -1;
		}
		_buf += sep_count * SECTOR_SIZE;
		offset += sep_count * SECTOR_SIZE;
		count -= sep_count * SECTOR_SIZE;
		read += sep_count * SECTOR_SIZE;
	}

	/* ending at no sector boundary? */
	if (count) {
		uint8_t tmp_buf[SECTOR_SIZE];

		rc = intf->read(dev, offset / SECTOR_SIZE, 1, tmp_buf);
		if (rc != 0) {
			dev_err(dev, "Cannot read data\n");
			return -1;
		}
		memcpy(_buf, tmp_buf, count);
		read += count;
	}

	return read;
}

static struct file_operations disk_ops = {
	.read  = disk_read,
	.write = disk_write,
	.lseek = dev_lseek_default,
};

/**
 * Probe the connected disk drive
 */
static int disk_probe(struct device_d *dev)
{
	uint8_t *sector;
	int rc;
	struct ata_interface *intf = dev->platform_data;
	struct cdev *disk_cdev;

	sector = xmalloc(SECTOR_SIZE);

	rc = intf->read(dev, 0, 1, sector);
	if (rc != 0) {
		dev_err(dev, "Cannot read MBR of this device\n");
		rc = -ENODEV;
		goto on_error;
	}

	/* It seems a valuable disk. Register it */
	disk_cdev = xzalloc(sizeof(struct cdev));

	/*
	 * BIOS based disks needs special handling. Not the driver can
	 * enumerate the hardware, the BIOS did it already. To show the user
	 * the drive ordering must not correspond to the Linux drive order,
	 * use the 'biosdisk' name instead.
	 */
#ifdef CONFIG_ATA_BIOS
	if (strcmp(dev->driver->name, "biosdisk") == 0)
		disk_cdev->name = asprintf("biosdisk%d", dev->id);
	else
#endif
		disk_cdev->name = asprintf("disk%d", dev->id);

	/* On x86, BIOS based disks are coming without a valid .size field */
	if (dev->size == 0) {
		/*
		 * We need always the size of the drive, else its nearly impossible
		 * to do anything with it (at least with the generic routines)
		 */
		disk_cdev->size = 32;
	} else
		disk_cdev->size = dev->size;
	disk_cdev->ops = &disk_ops;
	disk_cdev->dev = dev;
	devfs_create(disk_cdev);

	if ((sector[510] != 0x55) || (sector[511] != 0xAA)) {
		dev_info(dev, "No partition table found\n");
		rc = 0;
		goto on_error;
	}

	if (dev->size == 0) {
		/* guess the size of this drive if not otherwise given */
		dev->size = disk_guess_size(dev,
			(struct partition_entry*)&sector[446]) * SECTOR_SIZE;
		dev_info(dev, "Drive size guessed to %u kiB\n", dev->size / 1024);
		disk_cdev->size = dev->size;
	}

	rc = disk_register_partitions(dev, (struct partition_entry*)&sector[446]);

on_error:
	free(sector);
	return rc;
}

#ifdef CONFIG_ATA_BIOS
static struct driver_d biosdisk_driver = {
        .name   = "biosdisk",
        .probe  = disk_probe,
};
#endif

static struct driver_d disk_driver = {
        .name   = "disk",
        .probe  = disk_probe,
};

static int disk_init(void)
{
#ifdef CONFIG_ATA_BIOS
	register_driver(&biosdisk_driver);
#endif
	register_driver(&disk_driver);
	return 0;
}

device_initcall(disk_init);
