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
 *
 */

/**
 * @file
 * @brief Write the barebox binary to the MBR and the following disk sectors
 *
 * Also patch dedicated locations in the image to make it work at runtime
 *
 * Current restrictions are:
 * - only installs into MBR and the sectors after it
 * - tested only with QEMU
 * - and maybe some others
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

/* include the info from this barebox release */
#include "../../include/generated/utsrelease.h"
#include "../../arch/x86/mach-i386/include/mach/barebox.lds.h"

/** define to disable integrity tests and debug messages */
#define NDEBUG

/* some infos about our target architecture */
#include "arch.h"

/**
 * "Disk Address Packet Structure" to be used when calling int13,
 * function 0x42
 *
 * @note All entries are in target endianess
 */
struct DAPS
{
	uint8_t size;		/**< size of this data set, 0 marks it as invalid */
	uint8_t res1;		/**< always 0 */
	int8_t count;		/**< number of sectors 0...127 to handle */
	uint8_t res2;		/**< always 0 */
	uint16_t offset;	/**< store address: offset */
	uint16_t segment;	/**< store address: segment */
	uint64_t lba;		/**< start sector number in LBA */
} __attribute__ ((packed));

/**
 * Description of one partition table entry (D*S type)
 *
 * @note All entries are in target endianess
 */
struct partition_entry {
	uint8_t boot_indicator;
	uint8_t chs_begin[3];
	uint8_t type;
	uint8_t chs_end[3];
	uint32_t partition_start;	/* LE */
	uint32_t partition_size;	/* LE */
} __attribute__ ((packed));

#ifndef NDEBUG
static void debugout(const struct DAPS *entry, int supress_entry)
{
	if (supress_entry)
		printf("DAPS entry: ");
	else
		printf("DAPS entry % 3u: ", ((unsigned)entry & ( SECTOR_SIZE - 1)) / sizeof(struct DAPS));

	printf("Size: % 2u, Count: % 3d, Offset: 0x%04hX, Segment: 0x%04hX, LBA: %llu\n",
		entry->size, entry->count,
		target2host_16(entry->offset), target2host_16(entry->segment),
		target2host_64(entry->lba));
}
#else
# define debugout(x,y) (__ASSERT_VOID_CAST(0))
#endif

/**
 * Fill *one* DAPS
 * @param sector The DAPS to fill
 * @param count Sector count
 * @param offset Realmode offset in the segment
 * @param segment Real mode segment
 * @param lba LBA of the first sector to read
 * @return 0 on success
 */
static int fill_daps(struct DAPS *sector, unsigned count, unsigned offset, unsigned segment, uint64_t lba)
{
	assert(sector != NULL);
	assert(count < 128);
	assert(offset < 0x10000);
	assert(segment < 0x10000);

	sector->size = sizeof(struct DAPS);
	sector->res1 = 0;
	sector->count = (int8_t)count;
	sector->res2 = 0;
	sector->offset = host2target_16(offset);
	sector->segment = host2target_16(segment);
	sector->lba = host2target_64(lba);

	return 0;
}

/**
 * Mark a DAPS invalid to let the boot loader code stop at this entry.
 * @param sector The DAPS to be marked as invalid
 *
 * Marking as invalid must be done in accordance to the detection method
 * the assembler routine in the boot loader uses:
 * The current code tests for zero in the first two bytes of the DAPS.
 */
static void invalidate_daps(struct DAPS *sector)
{
	sector->size = MARK_DAPS_INVALID;
	sector->res1 = 0;
}

/**
 * Create the indirect sector with the DAPS entries
 * @param daps_table Where to store the entries
 * @param size Size of the whole image in bytes
 * @param pers_sector_count Count of sectors to skip after MBR for the persistant environment storage
 * @return 0 on success
 *
 * This routine calculates the DAPS entries for the case the whole
 * barebox images fits into the MBR itself and the sectors after it.
 * This means the start of the first partition must keep enough sectors
 * unused.
 * It also skips 'pers_sector_count' sectors after the MBR for special
 * usage if given.
 */
static int barebox_linear_image(struct DAPS *daps_table, off_t size, long pers_sector_count)
{
	unsigned offset = LOAD_AREA, next_offset;
	unsigned segment = LOAD_SEGMENT;
	unsigned chunk_size, i = 0;
	uint64_t lba = 2 + pers_sector_count;
	int rc;

	/*
	 * We can load up to 127 sectors in one chunk. What a bad number...
	 * So, we will load in chunks of 32 kiB.
	 */

	/* at runtime two sectors from the image are already loaded: MBR and indirect */
	size -= 2 * SECTOR_SIZE;
	/* and now round up to multiple of sector size */
	size = (size + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1);

	/*
	 * The largest image we can load with this method is:
	 * (SECTOR_SIZE / sizeof(DAPS) - 1) * 32 kiB
	 * For a 512 byte sector and a 16 byte DAPS:
	 * (512 / 16 - 1) * 32 kiB = 992 kiB
	 * Note: '- 1' to consider one entry is required to pad to a 32 kiB boundary
	 */

	if (size >= (SECTOR_SIZE / sizeof(struct DAPS) - 1) * 32 * 1024) {
		fprintf(stderr, "Image too large to boot. Max size is %zu kiB, image size is %lu kiB\n",
			(SECTOR_SIZE / sizeof(struct DAPS) - 1) * 32, size / 1024);
		return -1;
	}

	if (size > 32 * 1024) {
		/* first fill up until the next 32 k boundary */
		next_offset = (offset + 32 * 1024 -1) & ~0x7fff;
		chunk_size = next_offset - offset;
		if (chunk_size & (SECTOR_SIZE-1)) {
			fprintf(stderr, "Unable to pad from %X to %X in multiple of sectors\n", offset, next_offset);
			return -1;
		}

		rc = fill_daps(&daps_table[i], chunk_size / SECTOR_SIZE, offset, segment, lba);
		if (rc != 0)
			return -1;
		debugout(&daps_table[i], 0);

		/* calculate values to enter the loop for the other entries */
		size -= chunk_size;
		i++;
		lba += chunk_size / SECTOR_SIZE;
		offset += chunk_size;
		if (offset >= 0x10000) {
			segment += 4096;
			offset = 0;
		}

		/*
		 * Now load the remaining image part in 32 kiB chunks
		 */
		while (size) {
			if (size >= 32 * 1024 ) {
				if (i >= (SECTOR_SIZE / sizeof(struct DAPS))) {
					fprintf(stderr, "Internal tool error: Too many DAPS entries!\n");
					return -1;
				}
				rc = fill_daps(&daps_table[i], 64, offset, segment, lba);
				if (rc != 0)
					return -1;
				debugout(&daps_table[i], 0);

				size -= 32 * 1024;
				lba += 64;
				offset += 32 * 1024;
				if (offset >= 0x10000) {
					segment += 4096;
					offset = 0;
				}
				i++;
			} else {
				if (i >= (SECTOR_SIZE / sizeof(struct DAPS))) {
					fprintf(stderr, "Internal tool error: Too many DAPS entries!\n");
					return -1;
				}
				rc = fill_daps(&daps_table[i], size / SECTOR_SIZE, offset, segment, lba);
				if (rc != 0)
					return -1;
				debugout(&daps_table[i], 0);
				size = 0;	/* finished */
				i++;
			}
		};
	} else {
		/* less than 32 kiB. Very small image... */
		rc = fill_daps(&daps_table[i], size / SECTOR_SIZE, offset, segment, lba);
		if (rc != 0)
			return -1;
		debugout(&daps_table[i], 0);
		i++;
	}

	/*
	 * Do not mark an entry as invalid if the buffer is full. The
	 * boot code stops if all entries of a buffer are read.
	 */
	if (i >= (SECTOR_SIZE / sizeof(struct DAPS)))
		return 0;

	/* mark the last DAPS invalid */
	invalidate_daps(&daps_table[i]);
	debugout(&daps_table[i], 0);

	return 0;
}

/**
 * Do some simple sanity checks if this sector could be an MBR
 * @param sector Sector with data to check
 * @param size Size of the buffer
 * @return 0 if successfull
 */
static int check_for_valid_mbr(const uint8_t *sector, off_t size)
{
	if (size < SECTOR_SIZE) {
		fprintf(stderr, "MBR too small to be valid\n");
		return -1;
	}

	if ((sector[OFFSET_OF_SIGNATURE] != 0x55) ||
		(sector[OFFSET_OF_SIGNATURE + 1] != 0xAA)) {
		fprintf(stderr, "No MBR signature found\n");
		return -1;
	}

	/* FIXME: try to check if there is a valid partition table */
	return 0;
}

/**
 * Check space between start of the image and the start of the first partition
 * @param hd_image HD image to examine
 * @param size Size of the barebox image
 * @return 0 on success, -1 if the barebox image is too large
 */
static int check_for_space(const void *hd_image, off_t size)
{
	struct partition_entry *partition;
	uint8_t *mbr_disk_sector = (uint8_t*)hd_image;
	off_t spare_sector_count;

	assert(hd_image != NULL);
	assert(size > 0);

	if (check_for_valid_mbr(hd_image, size) != 0)
		return -1;

	/* where to read */
	partition = (struct partition_entry*) &mbr_disk_sector[OFFSET_OF_PARTITION_TABLE];

	/* TODO describes the first entry always the first partition? */
	spare_sector_count = target2host_32(partition->partition_start);

#ifdef DEBUG
	printf("Debug: Required free sectors for barebox prior first partition: %lu, hd image provides: %lu\n",
		(size + SECTOR_SIZE - 1) / SECTOR_SIZE, spare_sector_count);
#endif
	spare_sector_count *= SECTOR_SIZE;
	if (spare_sector_count < size) {
		fprintf(stderr, "Not enough space after MBR to store barebox\n");
		fprintf(stderr, "Move begin of the first partition beyond sector %lu\n", (size + SECTOR_SIZE - 1) / SECTOR_SIZE);
		return -1;
	}

	return 0;
}

/**
 * Setup the persistant environment storage information
 * @param patch_area Where to patch
 * @param pers_sector_start Start sector of the persistant environment storage
 * @param pers_sector_count Count of sectors for the persistant environment storage
 * @return 0 on success
 */
static int store_pers_env_info(void *patch_area, uint64_t pers_sector_start, long pers_sector_count)
{
	uint64_t *start_lba = (uint64_t*)(patch_area + PATCH_AREA_PERS_START);
	uint16_t *count_lba = (uint16_t*)(patch_area + PATCH_AREA_PERS_SIZE);

	assert(patch_area != NULL);
	assert(pers_sector_count >= 0);

	if (pers_sector_count == 0) {
		*count_lba = host2target_16(PATCH_AREA_PERS_SIZE_UNUSED);
		return 0;
	}

	*start_lba = host2target_64(pers_sector_start);
	*count_lba = host2target_16(pers_sector_count);

	return 0;
}

/**
 * Prepare the MBR and indirect sector for runtime
 * @param fd_barebox barebox image to use
 * @param fd_hd Hard disk image to prepare
 * @param pers_sector_count Count of sectors to skip after MBR for the persistant environment storage
 * @return 0 on success
 *
 * This routine expects a prepared hard disk image file with a partition table
 * in its first sector. This method only is currently supported.
 */
static int barebox_overlay_mbr(int fd_barebox, int fd_hd, long pers_sector_count)
{
	const void *barebox_image;
	void *hd_image;
	int rc;
	struct stat sb;
	struct DAPS *embed;	/* part of the MBR */
	struct DAPS *indirect;	/* sector with indirect DAPS */
	off_t required_size;

	if (fstat(fd_barebox, &sb) == -1) {
		perror("fstat");
		return -1;
	}

	/* the barebox image won't be touched */
	barebox_image = mmap(NULL, sb.st_size,  PROT_READ, MAP_SHARED, fd_barebox, 0);
	if (barebox_image == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	rc = check_for_valid_mbr(barebox_image, sb.st_size);
	if (rc != 0) {
		fprintf(stderr, "barebox image seems not valid: Bad MBR signature\n");
		goto on_error_hd;
	}

	/*
	 * the persistant environment storage is in front of the main
	 * barebox image. To handle both, we need more space in front of the
	 * the first partition.
	 */
	required_size = sb.st_size + pers_sector_count * SECTOR_SIZE;

	/* the hd image will be modified */
	hd_image = mmap(NULL, required_size,  PROT_READ | PROT_WRITE,
					MAP_SHARED, fd_hd, 0);
	if (hd_image == MAP_FAILED) {
		perror("mmap");
		rc = -1;
		goto on_error_hd;
	}

	/* check for space */
	rc = check_for_space(hd_image, required_size);
	if (rc != 0)
		goto on_error_space;

	/* embed barebox's boot code into the disk drive image */
	memcpy(hd_image, barebox_image, OFFSET_OF_PARTITION_TABLE);

	/*
	 * embed the barebox main image into the disk drive image,
	 * but keep the persistant environment storage untouched
	 * (if defined), e.g. store the main image behind this special area.
	 */
	memcpy(hd_image + ((pers_sector_count + 1) * SECTOR_SIZE),
			barebox_image + SECTOR_SIZE, sb.st_size - SECTOR_SIZE);

	/* now, prepare this hard disk image for BIOS based booting */
	embed = hd_image + PATCH_AREA;
	indirect = hd_image + ((pers_sector_count + 1) * SECTOR_SIZE);

	/*
	 * Fill the embedded DAPS to let the basic boot code find the
	 * indirect sector at runtime
	 */
#ifdef DEBUG
	printf("Debug: Fill in embedded DAPS\n");
#endif
	rc = fill_daps(embed, 1, INDIRECT_AREA, INDIRECT_SEGMENT,
				1 + pers_sector_count);
	if (rc != 0)
		goto on_error_space;
	debugout(embed, 1);

#ifdef DEBUG
	printf("Debug: Fill in indirect sector\n");
#endif
	/*
	 * fill the indirect sector with the remaining DAPS to load the
	 * whole barebox image at runtime
	 */
	rc = barebox_linear_image(indirect, sb.st_size, pers_sector_count);
	if (rc != 0)
		goto on_error_space;

	/*
	 * TODO: Replace the fixed LBA starting number by a calculated one,
	 * to support barebox as a chained loader in a different start
	 * sector than the MBR
	 */
	rc = store_pers_env_info(embed, 1, pers_sector_count);
	if (rc != 0)
		goto on_error_space;

on_error_space:
	munmap(hd_image, required_size);

on_error_hd:
	munmap((void*)barebox_image, sb.st_size);

	return rc;
}

static void print_usage(const char *pname)
{
	printf("%s: Preparing a hard disk image for boot with barebox on x86.\n", pname);
	printf("Usage is\n %s [options] -m <barebox image> -d <hd image>\n", pname);
	printf(" [options] are:\n -s <count> sector count of the persistant environment storage\n");
	printf(" <barebox image> barebox's boot image file\n");
	printf(" <hd image> HD image to store the barebox image\n");
	printf(" If no '-s <x>' was given, barebox occupies sectors 0 to n, else sector 0 and x+1 to n\n");
}

int main(int argc, char *argv[])
{
	int rc = 0, c;
	char *barebox_image_filename = NULL, *hd_image_filename = NULL;
	int fd_barebox_image = 0, fd_hd_image = 0;
	long barebox_pers_size = -1;

	if (argc == 1) {
		print_usage(argv[0]);
		exit(0);
	}

	/* handle command line options first */
	while (1) {
		c = getopt(argc, argv, "m:d:s:hv");
		if (c == -1)
			break;

		switch (c) {
		case 's':
			barebox_pers_size = strtol(optarg, NULL, 0);
			break;
		case 'm':
			barebox_image_filename = strdup(optarg);
			break;
		case 'd':
			hd_image_filename = strdup(optarg);
			break;
		case 'h':
			print_usage(argv[0]);
			rc = 0;
			goto on_error;
		case 'v':
			printf("setupmbr for u-boot-v%s\n", UTS_RELEASE);
			printf("Send bug reports to 'barebox@lists.infradead.org'\n");
			rc = 0;
			goto on_error;
		}
	}

	if (barebox_image_filename == NULL) {
		print_usage(argv[0]);
		rc = -1;
		goto on_error;
	}

	fd_barebox_image = open(barebox_image_filename, O_RDONLY);
	if (fd_barebox_image == -1) {
		fprintf(stderr, "Cannot open '%s' for reading\n",
				barebox_image_filename);
		rc = -1;
		goto on_error;
	}

	fd_hd_image = open(hd_image_filename, O_RDWR);
	if (fd_hd_image == -1) {
		fprintf(stderr, "Cannot open '%s'\n", hd_image_filename);
		rc = -1;
		goto on_error;
	}

	if (barebox_pers_size < 0)
		barebox_pers_size = 0;

	rc = barebox_overlay_mbr(fd_barebox_image, fd_hd_image, barebox_pers_size);

on_error:
	if (fd_barebox_image != -1)
		close(fd_barebox_image);
	if (fd_hd_image != -1)
		close(fd_hd_image);

	if (barebox_image_filename != NULL)
		free(barebox_image_filename);
	if (hd_image_filename != NULL)
		free(hd_image_filename);

	return rc;
}

/** @page x86_bootloader barebox acting as PC bootloader

@section x86_bootloader_features Features

@a barebox can act as a bootloader for PC based systems. In this case a special
binary layout will be created to be able to store it on some media the PC
BIOS can boot from. It can boot Linux kernels stored also on the same boot
media and be configured at runtime, with the possibility to store the changed
configuration on the boot media.

@section x86_bootloader_restrictions Restrictions

Due to some BIOS and @a barebox restrictions the boot media must be
prepared in some special way:

@li @a barebox must be stored in the MBR (Master Boot Record) of the boot media.
    Currently its not possible to store and boot it in one of the partition
    sectors (to use it as a second stage loader). This is no eternal
    restriction. It only needs further effort to add this feature.
@li @a barebox currently cannot run a chained boot loader. Also, this is no
    eternal restriction, only further effort needed.
@li @a barebox comes with limited filesystem support. There is currently no
    support for the most common and popular filesystems used in the *NIX world.
    This restricts locations where to store a kernel and other runtime
    information
@li @a barebox must be stored to the first n sectors of the boot media. To ensure
    this does not collide with partitions on the boot media, the first
    partition must start at a sector behind the ones @a barebox occupies.
@li @a barebox handles its runtime configuration in a special way: It stores it
    in a binary way into some reserved sectors ("persistant storage").

@section x86_bootloader_preparations Boot Preparations

To store the @a barebox image to a boot media, it comes with the tool
@p setupmbr in the directory @p scripts/setupmbr/ . To be able to use it on
the boot media of your choice, some preparations are required:

@subsection x86_bootloader_preparations_partition Keep Sectors free

Build the @a barebox image and check its size. At least this amount of
sectors must be kept free after the MBR prior the first partition. Do this
simple calulation:

	sectors = (\<size of barebox image\> + 511) / 512

To be able to store the runtime configuration, further free sectors are
required. Its up to you and your requirements, how large this persistant
storage must be. If you need 16 kiB for this purpose, you need to keep
additional 32 sectors free.

For this example we are reserving 300 sectors for the @a barebox image and
additionaly 32 sectors for the persistant storage. So, the first partition on
the boot media must start at sector 333 or later.

Run the @p fdisk tool to setup such a partition table:

@verbatim
[jb@host]~> fdisk /dev/sda
Command (m for help): p

Disk /dev/sda: 20.7 MB, 212680704 bytes
16 heads, 63 sectors/track, 406 cylinders
Units = cylinders of 1008 * 512 = 516096 bytes

   Device Boot      Start         End      Blocks   Id  System
@endverbatim

Change the used units to @p sectors for easier handling.

@verbatim
Command (m for help): u
Changing display/entry units to sectors

Command (m for help): p

Disk /dev/sda: 20.7 MB, 212680704 bytes
16 heads, 63 sectors/track, 406 cylinders, total 409248 sectors
Units = sectors of 1 * 512 = 512 bytes

   Device Boot      Start         End      Blocks   Id  System
@endverbatim

Now its possible to create the first partition with the required offset:

@verbatim
Command (m for help): n
Command action
   e   extended
   p   primary partition (1-4)
p
Partition number (1-4): 1
First sector (63-409247, default 63): 333
Last sector or +size or +sizeM or +sizeK (333-409247, default 409247): +18M
Command (m for help): p

Disk /dev/sda: 20.7 MB, 212680704 bytes
16 heads, 63 sectors/track, 406 cylinders, total 409248 sectors
Units = sectors of 1 * 512 = 512 bytes

        Device Boot      Start         End      Blocks   Id  System
/dev/sda                   333       35489       17578+  83  Linux
@endverbatim

That's all. Do whatever is required now with the new partition (formatting
and populating the root filesystem for example) to make it useful.

In the next step, @a barebox gets installed to this boot media:

@verbatim
[jb@host]~> scripts/setupmbr/setupmbr -s 32 -m ./barebox -d /dev/sda
@endverbatim

This command writes the @a barebox image file './barebox' onto the device
@p /dev/sda.

The @p -s option will keep the persistant storage sectors free and untouched
and set flags in the MBR to forward its existance, size and location to
@a barebox at runtime. @p setupmbr also does not change the partition table.

The @a barebox image gets stored on the boot media like this:

@verbatim
sector 0   1             33                              333
       |---|-------------|--------------- ~~~ ------------|--------------
      MBR    persistant              barebox                 first
              storage               main image              partition
@endverbatim

If the @p -s option is omitted, the "persistant storage" part simply does
not exist:

@verbatim
sector 0   1                              333
       |---|--------------- ~~~ ------------|--------------
      MBR               barebox                 first
                       main image              partition
@endverbatim

@note The @p setupmbr tool is also working on real image file than on device
      nodes only. So, there is no restriction what kind of file will be
      modified.
*/
