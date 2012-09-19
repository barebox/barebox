/*
 * Copyright (C) 2009...2011 Juergen Beisert, Pengutronix
 *
 * Mostly stolen from the GRUB2 project
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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

/**
 * @file
 * @brief Media communication layer through the standard 16 bit PC-BIOS
 *
 * This communication driver does all accesses to the boot medium via 16 bit
 * real mode calls into the standard BIOS. Due to this method, its possible
 * to use all the medias to boot from that are supported by the BIOS. This
 * also includes emulated only medias.
 *
 * To be able to call the real mode BIOS, this driver must switch back to
 * real mode for each access. This will slow down the access a little bit, but
 * we are a boot loader here, not an operating system...
 *
 * Note: We need scratch memory for the BIOS communication, because the BIOS
 * can only handle memory below 0xA0000. So we must copy all data between
 * the flat mode buffers and realmode buffers.
 *
 * Note: This driver makes no sense on other architectures than x86.
 *
 * Note: This driver does only support LBA addressing. Currently no CHS!
 */

#include <common.h>
#include <init.h>
#include <asm/syslib.h>
#include <errno.h>
#include <block.h>
#include <disks.h>
#include <malloc.h>

/**
 * Sector count handled in one count
 *
 * @todo 127 are always possible, some BIOS manufacturer supports up to 255.
 * Is it's worth to detect Phoenic's restriction?
 */
#define SECTORS_AT_ONCE 64

/** Command to read sectors from media */
#define BIOS_READ_CMD 0

/** Command to write sectors to media */
#define BIOS_WRT_CMD 1

/**
 * "Disk Address Packet Structure" to be used when calling
 * BIOS's int13, function 0x42/0x43
 */
struct DAPS
{
	uint8_t size;		/**< always '16' */
	uint8_t res1;		/**< always '0' */
	int8_t count;		/**< number of sectors 0...127 */
	uint8_t res2;		/**< always '0' */
	uint16_t offset;	/**< buffer address: offset */
	uint16_t segment;	/**< buffer address: segment */
	uint64_t lba;		/**< LBA of the start sector */
} __attribute__ ((packed));

/**
 * Collection of data we need to know about the connected drive
 */
struct media_access {
	struct block_device blk; /**< the main device */
	int drive_no;	/**< drive number used by the BIOS */
	int is_cdrom;	/**< drive is a CDROM e.g. no write support */
};

#define to_media_access(x) container_of((x), struct media_access, blk)

/**
 * Scratch memory for BIOS communication to handle data in chunks of 32 kiB
 *
 * Note: This variable is located in the .bss segment, assuming it is located
 * below 0xA0000. If not, the BIOS is not able to read or store any data
 * from/to it. The variable must also aligned to a 16 byte boundary to easify
 * linear to segment:offset address conversion.
 */
static uint8_t scratch_buffer[SECTORS_AT_ONCE * SECTOR_SIZE] __attribute__((aligned(16)));

/**
 * Communication buffer for the 16 bit int13 BIOS call
 *
 * Note: This variable is located in the .bss segment, assuming it is located
 * below 0xA0000. If not, the BIOS is not able to read or store any data
 * from/to it. The variable must also aligned to a 16 byte boundary to easify
 * linear to segment:offset conversion.
 */
static struct DAPS bios_daps __attribute__((aligned(16)));

/**
 * @param media our data we need to do the access
 * @param cmd Command to forward to the BIOS
 * @param sector_start LBA of the start sector
 * @param sector_count Sector count
 * @param buffer Buffer to read from or write to (in the low memory area)
 * @return 0 on success, anything else on failure
 */
static int biosdisk_bios_call(struct media_access *media, int cmd, uint64_t sector_start, unsigned sector_count, void *buffer)
{
	int rc;

	/* prepare the DAPS for the int13 call */
	bios_daps.size = sizeof(struct DAPS);
	bios_daps.res1 = 0;
	bios_daps.count = sector_count;	/* always less than 128! */
	bios_daps.res2 = 0;
	bios_daps.segment = (unsigned long)buffer >> 4;
	bios_daps.offset = (unsigned long)buffer - (unsigned long)(bios_daps.segment << 4);
	bios_daps.lba = sector_start;

	if (cmd == BIOS_READ_CMD)
		rc = bios_disk_rw_int13_extensions(0x42, media->drive_no, &bios_daps);
	else if (cmd == BIOS_WRT_CMD)
		rc = bios_disk_rw_int13_extensions(0x43, media->drive_no, &bios_daps);
	else
		return -1;

	return rc;
}

/**
 * Read a chunk of sectors from media
 * @param blk All info about the block device we need
 * @param buffer Buffer to read into
 * @param block Sector's LBA number to start read from
 * @param num_blocks Sector count to read
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to store all data!
 *
 * @note Due to 'block' is of type 'int' only small disks can be handled!
 */
static int biosdisk_read(struct block_device *blk, void *buffer, int block,
				int num_blocks)
{
	int rc;
	uint64_t sector_start = block;
	unsigned sector_count = num_blocks;
	struct media_access *media = to_media_access(blk);

	while (sector_count >= SECTORS_AT_ONCE) {
		rc = biosdisk_bios_call(media, BIOS_READ_CMD, sector_start, SECTORS_AT_ONCE, scratch_buffer);
		if (rc != 0)
			return rc;
		__builtin_memcpy(buffer, scratch_buffer, sizeof(scratch_buffer));
		buffer += sizeof(scratch_buffer);
		sector_start += SECTORS_AT_ONCE;
		sector_count -= SECTORS_AT_ONCE;
	};

	/* Are sectors still remaining? */
	if (sector_count) {
		rc = biosdisk_bios_call(media, BIOS_READ_CMD, sector_start, sector_count, scratch_buffer);
		__builtin_memcpy(buffer, scratch_buffer, sector_count * SECTOR_SIZE);
	} else
		rc = 0;

	return rc;
}

/**
 * Write a chunk of sectors to media
 * @param blk All info about the block device we need
 * @param buffer Buffer to write from
 * @param block Sector's LBA number to start write to
 * @param num_blocks Sector count to write
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to read all data!
 *
 * @note Due to 'block' is of type 'int' only small disks can be handled!
 */
static int __maybe_unused biosdisk_write(struct block_device *blk,
				const void *buffer, int block, int num_blocks)
{
	int rc;
	uint64_t sector_start = block;
	unsigned sector_count = num_blocks;
	struct media_access *media = to_media_access(blk);

	while (sector_count >= SECTORS_AT_ONCE) {
		__builtin_memcpy(scratch_buffer, buffer, sizeof(scratch_buffer));
		rc = biosdisk_bios_call(media, BIOS_WRT_CMD, sector_start, SECTORS_AT_ONCE, scratch_buffer);
		if (rc != 0)
			return rc;
		buffer += sizeof(scratch_buffer);
		sector_start += SECTORS_AT_ONCE;
		sector_count -= SECTORS_AT_ONCE;
	};

	/* Are sectors still remaining? */
	if (sector_count) {
		__builtin_memcpy(scratch_buffer, buffer, sector_count * SECTOR_SIZE);
		rc = biosdisk_bios_call(media, BIOS_WRT_CMD, sector_start, sector_count, scratch_buffer);
	} else
		rc = 0;

	return rc;
}

static struct block_device_ops bios_ata = {
	.read = biosdisk_read,
#ifdef CONFIG_BLOCK_WRITE
	.write = biosdisk_write,
#endif
};

/**
 * Probe for connected drives and register them
 *
 * Detecting if a drive is present is done by simply reading its MBR.
 *
 * FIXME: Relation between BIOS disk numbering scheme and our representation
 * here in barebox (and later on in the linux kernel)
 */
static int biosdisk_probe(struct device_d *dev)
{
	int drive, rc;
	struct media_access media, *m;

	for (drive = 0x80; drive < 0x90; drive++) {
		media.drive_no = drive;
		media.is_cdrom = 0;	/* don't know yet */
		rc = biosdisk_bios_call(&media, BIOS_READ_CMD, 0, 1, scratch_buffer);
		if (rc != 0)
			continue;

		printf("BIOSdrive %d seems valid. Registering...\n", media.drive_no);

		m = xzalloc(sizeof(struct media_access));

		m->blk.dev = dev;
		m->blk.ops = &bios_ata;
		/*
		 * keep the 'blk.num_blocks' member 0, as we don't know
		 * the size of this disk yet!
		 */
		rc = cdev_find_free_index("disk");
		if (rc < 0)
			pr_err("Cannot find a free number for the disk node\n");
		m->blk.cdev.name = asprintf("disk%d", rc);
		m->blk.blockbits = SECTOR_SHIFT;

		rc = blockdevice_register(&m->blk);
		if (rc != 0) {
			dev_err(dev, "Cannot register BIOSdrive %d\n",
						media.drive_no);
			free(m);
			return rc;
		}

		/* create partitions on demand */
		rc = parse_partition_table(&m->blk);
		if (rc != 0)
			dev_warn(dev, "No partition table found\n");
	}

	return 0;
}

static struct driver_d biosdisk_driver = {
        .name   = "biosdrive",
        .probe  = biosdisk_probe,
};

static int biosdisk_init(void)
{
	/* sanity */
	if (scratch_buffer > (uint8_t*)0x9FFFF) {
		printf("BIOS driver: Scratch memory not in real mode area. Cannot continue!\n");
		return -EIO;
	}
	if (&bios_daps > (struct DAPS*)0x9FFFF) {
		printf("BIOS driver: DAPS memory not in real mode area. Cannot continue!\n");
		return -EIO;
	}

	platform_driver_register(&biosdisk_driver);
	return 0;
}

device_initcall(biosdisk_init);
