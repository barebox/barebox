/*
 * (C) Copyright 2002-2004
 * Brad Kemp, Seranoa Networks, Brad.Kemp@seranoa.com
 *
 * Copyright (C) 2003 Arabella Software Ltd.
 * Yuli Barcohen <yuli@arabellasw.com>
 *
 * Copyright (C) 2004
 * Ed Okerson
 *
 * Copyright (C) 2006
 * Tolunay Orkun <listmember@orkun.us>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 *
 */

/* The DEBUG define must be before common to enable debugging */
/* #define DEBUG	*/

#include <common.h>
#include <asm/byteorder.h>
#include <environment.h>
#include <clock.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <errno.h>
#include <progress.h>
#include "cfi_flash.h"

/*
 * This file implements a Common Flash Interface (CFI) driver for barebox.
 * The width of the port and the width of the chips are determined at initialization.
 * These widths are used to calculate the address for access CFI data structures.
 *
 * References
 * JEDEC Standard JESD68 - Common Flash Interface (CFI)
 * JEDEC Standard JEP137-A Common Flash Interface (CFI) ID Codes
 * Intel Application Note 646 Common Flash Interface (CFI) and Command Sets
 * Intel 290667-008 3 Volt Intel StrataFlash Memory datasheet
 * AMD CFI Specification, Release 2.0 December 1, 2001
 * AMD/Spansion Application Note: Migration from Single-byte to Three-byte
 *   Device IDs, Publication Number 25538 Revision A, November 8, 2001
 *
 */

static uint flash_offset_cfi[2]={FLASH_OFFSET_CFI,FLASH_OFFSET_CFI_ALT};

/*
 * Check if chip width is defined. If not, start detecting with 8bit.
 */
#ifndef CFG_FLASH_CFI_WIDTH
#define CFG_FLASH_CFI_WIDTH	FLASH_CFI_8BIT
#endif


/*
 * Functions
 */

static void flash_add_byte (struct flash_info *info, cfiword_t * cword, uchar c)
{
	if (bankwidth_is_1(info)) {
		*cword = c;
		return;
	}

#ifdef __BIG_ENDIAN
	*cword = (*cword << 8) | c;
#elif defined __LITTLE_ENDIAN

	if (bankwidth_is_2(info))
		*cword = (*cword >> 8) | (u16)c << 8;
	else if (bankwidth_is_4(info))
		*cword = (*cword >> 8) | (u32)c << 24;
	else if (bankwidth_is_8(info))
		*cword = (*cword >> 8) | (u64)c << 56;
#else
#error "could not determine byte order"
#endif
}

static int flash_write_cfiword (struct flash_info *info, ulong dest,
				cfiword_t cword)
{
	void *dstaddr = (void *)dest;
	int flag;

	/* Check if Flash is (sufficiently) erased */
	if (bankwidth_is_1(info))
		flag = ((flash_read8(dstaddr) & cword) == cword);
	else if (bankwidth_is_2(info))
		flag = ((flash_read16(dstaddr) & cword) == cword);
	else if (bankwidth_is_4(info))
		flag = ((flash_read32(dstaddr) & cword) == cword);
	else if (bankwidth_is_8(info))
		flag = ((flash_read64(dstaddr) & cword) == cword);
	else
		return 2;

	if (!flag)
		return 2;

	info->cfi_cmd_set->flash_prepare_write(info);

	flash_write_word(info, cword, (void *)dest);

	return flash_status_check (info, find_sector (info, dest),
					info->write_tout, "write");
}

#ifdef DEBUG
/*
 * Debug support
 */
void print_longlong (char *str, unsigned long long data)
{
	int i;
	char *cp;

	cp = (unsigned char *) &data;
	for (i = 0; i < 8; i++)
		sprintf (&str[i * 2], "%2.2x", *cp++);
}

static void flash_printqry (struct cfi_qry *qry)
{
	u8 *p = (u8 *)qry;
	int x, y;
	unsigned char c;

	for (x = 0; x < sizeof(struct cfi_qry); x += 16) {
		debug("%02x : ", x);
		for (y = 0; y < 16; y++)
			debug("%2.2x ", p[x + y]);
		debug(" ");
		for (y = 0; y < 16; y++) {
			c = p[x + y];
			if (c >= 0x20 && c <= 0x7e)
				debug("%c", c);
			else
				debug(".");
		}
		debug("\n");
	}
}
#endif

/*
 * read a character at a port width address
 */
uchar flash_read_uchar (struct flash_info *info, uint offset)
{
	uchar *cp = flash_make_addr(info, 0, offset);
#if defined __LITTLE_ENDIAN
	return flash_read8(cp);
#else
	return flash_read8(cp + info->portwidth - 1);
#endif
}

/*
 * read a long word by picking the least significant byte of each maximum
 * port size word. Swap for ppc format.
 */
static ulong flash_read_long (struct flash_info *info, flash_sect_t sect, uint offset)
{
	uchar *addr;
	ulong retval;

#ifdef DEBUG
	int x;
#endif
	addr = flash_make_addr (info, sect, offset);

#ifdef DEBUG
	dev_dbg(info->dev, "long addr is at %p info->portwidth = %d\n", addr,
	       info->portwidth);
	for (x = 0; x < 4 * info->portwidth; x++) {
		dev_dbg(info->dev, "addr[%x] = 0x%x\n", x, flash_read8(addr + x));
	}
#endif
#if defined __LITTLE_ENDIAN
	retval = ((flash_read8(addr) << 16) |
		  (flash_read8(addr + info->portwidth) << 24) |
		  (flash_read8(addr + 2 * info->portwidth)) |
		  (flash_read8(addr + 3 * info->portwidth) << 8));
#else
	retval = ((flash_read8(addr + 2 * info->portwidth - 1) << 24) |
		  (flash_read8(addr + info->portwidth - 1) << 16) |
		  (flash_read8(addr + 4 * info->portwidth - 1) << 8) |
		  (flash_read8(addr + 3 * info->portwidth - 1)));
#endif
	return retval;
}

/*
 * detect if flash is compatible with the Common Flash Interface (CFI)
 * http://www.jedec.org/download/search/jesd68.pdf
 *
*/
u32 jedec_read_mfr(struct flash_info *info)
{
	int bank = 0;
	uchar mfr;

	/* According to JEDEC "Standard Manufacturer's Identification Code"
	 * (http://www.jedec.org/download/search/jep106W.pdf)
	 * several first banks can contain 0x7f instead of actual ID
	 */
	do {
		mfr = flash_read_uchar (info,
				(bank << 8) | FLASH_OFFSET_MANUFACTURER_ID);
		bank++;
	} while (mfr == FLASH_ID_CONTINUATION);

	return mfr;
}

static void flash_read_cfi (struct flash_info *info, void *buf,
		unsigned int start, size_t len)
{
	u8 *p = buf;
	unsigned int i;

	for (i = 0; i < len; i++)
		p[i] = flash_read_uchar(info, start + i);
}

static int flash_detect_width (struct flash_info *info, struct cfi_qry *qry)
{
	int cfi_offset;


	for (info->portwidth = CFG_FLASH_CFI_WIDTH;
	     info->portwidth <= FLASH_CFI_64BIT; info->portwidth <<= 1) {
		for (info->chipwidth = FLASH_CFI_BY8;
		     info->chipwidth <= info->portwidth;
		     info->chipwidth <<= 1) {
			flash_write_cmd (info, 0, 0, AMD_CMD_RESET);
			flash_write_cmd (info, 0, 0, FLASH_CMD_RESET);
			for (cfi_offset=0; cfi_offset < sizeof(flash_offset_cfi)/sizeof(uint); cfi_offset++) {
				flash_write_cmd (info, 0, flash_offset_cfi[cfi_offset], FLASH_CMD_CFI);
				if (flash_isequal (info, 0, FLASH_OFFSET_CFI_RESP, 'Q')
				 && flash_isequal (info, 0, FLASH_OFFSET_CFI_RESP + 1, 'R')
				 && flash_isequal (info, 0, FLASH_OFFSET_CFI_RESP + 2, 'Y')) {
					flash_read_cfi(info, qry, FLASH_OFFSET_CFI_RESP,
						       sizeof(struct cfi_qry));
					info->interface = le16_to_cpu(qry->interface_desc);

					info->cfi_offset=flash_offset_cfi[cfi_offset];
					dev_dbg(info->dev, "device interface is %d\n",
						info->interface);
					dev_dbg(info->dev, "found port %d chip %d chip_lsb %d ",
						info->portwidth, info->chipwidth, info->chip_lsb);
					dev_dbg(info->dev, "port %d bits chip %d bits\n",
						info->portwidth << CFI_FLASH_SHIFT_WIDTH,
						info->chipwidth << CFI_FLASH_SHIFT_WIDTH);
					return 1;
				}
			}
		}
	}
	dev_dbg(info->dev, "not found\n");
	return 0;
}

static int flash_detect_cfi (struct flash_info *info, struct cfi_qry *qry)
{
	int ret;

	dev_dbg(info->dev, "flash detect cfi\n");

	info->chip_lsb = 0;
	ret = flash_detect_width (info, qry);
	if (!ret) {
		info->chip_lsb = 1;
		ret = flash_detect_width (info, qry);
	}
	return ret;
}

/*
 * The following code cannot be run from FLASH!
 */
static ulong flash_get_size (struct flash_info *info)
{
	int i, j;
	flash_sect_t sect_cnt;
	unsigned long sector;
	unsigned long tmp;
	int size_ratio;
	uchar num_erase_regions;
	int erase_region_size;
	int erase_region_count;
	int cur_offset = 0;
	struct cfi_qry qry;
	unsigned long base = (unsigned long)info->base;

	memset(&qry, 0, sizeof(qry));

	info->ext_addr = 0;
	info->cfi_version = 0;
#ifdef CFG_FLASH_PROTECTION
	info->legacy_unlock = 0;
#endif

	/* first only malloc space for the first sector */
	info->start = xmalloc(sizeof(ulong));

	info->start[0] = base;
	info->protect = 0;

	if (flash_detect_cfi (info, &qry)) {
		info->vendor = le16_to_cpu(qry.p_id);
		info->ext_addr = le16_to_cpu(qry.p_adr);
		num_erase_regions = qry.num_erase_regions;

		if (info->ext_addr) {
			info->cfi_version = (ushort) flash_read_uchar (info,
						info->ext_addr + 3) << 8;
			info->cfi_version |= (ushort) flash_read_uchar (info,
						info->ext_addr + 4);
		}

#ifdef DEBUG
		flash_printqry (&qry);
#endif

		switch (info->vendor) {
#ifdef CONFIG_DRIVER_CFI_INTEL
		case CFI_CMDSET_INTEL_EXTENDED:
		case CFI_CMDSET_INTEL_STANDARD:
			info->cfi_cmd_set = &cfi_cmd_set_intel;
			break;
#endif
#ifdef CONFIG_DRIVER_CFI_AMD
		case CFI_CMDSET_AMD_STANDARD:
		case CFI_CMDSET_AMD_EXTENDED:
			info->cfi_cmd_set = &cfi_cmd_set_amd;
			break;
#endif
		default:
			dev_err(info->dev, "unsupported vendor\n");
			return 0;
		}
		info->cfi_cmd_set->flash_read_jedec_ids (info);
		flash_write_cmd (info, 0, info->cfi_offset, FLASH_CMD_CFI);

		info->cfi_cmd_set->flash_fixup (info, &qry);

		dev_dbg(info->dev, "manufacturer is %d\n", info->vendor);
		dev_dbg(info->dev, "manufacturer id is 0x%x\n", info->manufacturer_id);
		dev_dbg(info->dev, "device id is 0x%x\n", info->device_id);
		dev_dbg(info->dev, "device id2 is 0x%x\n", info->device_id2);
		dev_dbg(info->dev, "cfi version is 0x%04x\n", info->cfi_version);

		size_ratio = info->portwidth / info->chipwidth;
		/* if the chip is x8/x16 reduce the ratio by half */
		if ((info->interface == FLASH_CFI_X8X16)
		    && (info->chipwidth == FLASH_CFI_BY8)
		    && (size_ratio != 1)) {
			size_ratio >>= 1;
		}
		dev_dbg(info->dev, "size_ratio %d port %d bits chip %d bits\n",
		       size_ratio, info->portwidth << CFI_FLASH_SHIFT_WIDTH,
		       info->chipwidth << CFI_FLASH_SHIFT_WIDTH);
		dev_dbg(info->dev, "found %d erase regions\n", num_erase_regions);
		info->eraseregions = xzalloc(sizeof(*(info->eraseregions)) * num_erase_regions);
		info->numeraseregions = num_erase_regions;
		sect_cnt = 0;
		sector = base;

		for (i = 0; i < num_erase_regions; i++) {
			struct mtd_erase_region_info *region = &info->eraseregions[i];

			if (i > NUM_ERASE_REGIONS) {
				dev_info(info->dev, "%d erase regions found, only %d used\n",
					num_erase_regions, NUM_ERASE_REGIONS);
				break;
			}

			tmp = le32_to_cpu(qry.erase_region_info[i]);
			dev_dbg(info->dev, "erase region %u: 0x%08lx\n", i, tmp);

			erase_region_count = (tmp & 0xffff) + 1;
			tmp >>= 16;
			erase_region_size =
				(tmp & 0xffff) ? ((tmp & 0xffff) * 256) : 128;
			dev_dbg(info->dev, "erase_region_count = %d erase_region_size = %d\n",
				erase_region_count, erase_region_size);

			region->offset = cur_offset;
			region->erasesize = erase_region_size;
			region->numblocks = erase_region_count;
			cur_offset += erase_region_size * erase_region_count;

			/* increase the space malloced for the sector start addresses */
			info->start = xrealloc(info->start, sizeof(ulong) * (erase_region_count + sect_cnt));
			info->protect = xrealloc(info->protect, sizeof(uchar) * (erase_region_count + sect_cnt));

			for (j = 0; j < erase_region_count; j++) {
				info->start[sect_cnt] = sector;
				sector += (erase_region_size * size_ratio);

				/*
				 * Only read protection status from supported devices (intel...)
				 */
				switch (info->vendor) {
				case CFI_CMDSET_INTEL_EXTENDED:
				case CFI_CMDSET_INTEL_STANDARD:
					info->protect[sect_cnt] =
						flash_isset (info, sect_cnt,
							     FLASH_OFFSET_PROTECT,
							     FLASH_STATUS_PROTECT);
					break;
				default:
					info->protect[sect_cnt] = 0; /* default: not protected */
				}

				sect_cnt++;
			}
		}

		info->sector_count = sect_cnt;
		/* multiply the size by the number of chips */
		info->size = (1 << qry.dev_size) * size_ratio;
		info->buffer_size = (1 << le16_to_cpu(qry.max_buf_write_size));
		info->erase_blk_tout = 1 << (qry.block_erase_timeout_typ +
					     qry.block_erase_timeout_max);
		info->buffer_write_tout = 1 << (qry.buf_write_timeout_typ +
						qry.buf_write_timeout_max);
		info->write_tout = 1 << (qry.word_write_timeout_typ +
					 qry.word_write_timeout_max);
		info->flash_id = FLASH_MAN_CFI;
		if ((info->interface == FLASH_CFI_X8X16) && (info->chipwidth == FLASH_CFI_BY8)) {
			info->portwidth >>= 1;	/* XXX - Need to test on x8/x16 in parallel. */
		}
		flash_write_cmd (info, 0, 0, info->cmd_reset);
	}

	return info->size;
}

/* loop through the sectors from the highest address
 * when the passed address is greater or equal to the sector address
 * we have a match
 */
flash_sect_t find_sector (struct flash_info *info, ulong addr)
{
	flash_sect_t sector;

	for (sector = info->sector_count - 1; sector >= 0; sector--) {
		if (addr >= info->start[sector])
			break;
	}
	return sector;
}

static int cfi_erase(struct flash_info *finfo, size_t count, loff_t offset)
{
        unsigned long start, end;
        int i, ret = 0;

	dev_dbg(finfo->dev, "%s: erase 0x%08llx (size %zu)\n", __func__, offset, count);

        start = find_sector(finfo, (unsigned long)finfo->base + offset);
        end   = find_sector(finfo, (unsigned long)finfo->base + offset +
			count - 1);

        for (i = start; i <= end; i++) {
                ret = finfo->cfi_cmd_set->flash_erase_one(finfo, i);
                if (ret)
                        goto out;

		if (ctrlc()) {
			ret = -EINTR;
			goto out;
		}
        }
out:
        return ret;
}

/*
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */
static int write_buff (struct flash_info *info, const uchar * src, ulong addr, ulong cnt)
{
	ulong wp;
	uchar *p;
	int aln;
	cfiword_t cword;
	int i, rc;

#ifdef CONFIG_CFI_BUFFER_WRITE
	int buffered_size;
#endif
	/* get lower aligned address */
	wp = (addr & ~(info->portwidth - 1));

	/* handle unaligned start */
	if ((aln = addr - wp) != 0) {
		cword = 0;
		p = (uchar*)wp;
		for (i = 0; i < aln; ++i)
			flash_add_byte (info, &cword, flash_read8(p + i));

		for (; (i < info->portwidth) && (cnt > 0); i++) {
			flash_add_byte (info, &cword, *src++);
			cnt--;
		}
		for (; (cnt == 0) && (i < info->portwidth); ++i)
			flash_add_byte (info, &cword, flash_read8(p + i));

		rc = flash_write_cfiword (info, wp, cword);
		if (rc != 0)
			return rc;

		wp += i;
	}

	/* handle the aligned part */
#ifdef CONFIG_CFI_BUFFER_WRITE
	buffered_size = (info->portwidth / info->chipwidth);
	buffered_size *= info->buffer_size;
	while (cnt >= info->portwidth) {
		/* prohibit buffer write when buffer_size is 1 */
		if (info->buffer_size == 1) {
			cword = 0;
			for (i = 0; i < info->portwidth; i++)
				flash_add_byte (info, &cword, *src++);
			if ((rc = flash_write_cfiword (info, wp, cword)) != 0)
				return rc;
			wp += info->portwidth;
			cnt -= info->portwidth;
			continue;
		}

		/* write buffer until next buffered_size aligned boundary */
		i = buffered_size - (wp % buffered_size);
		if (i > cnt)
			i = cnt;
		if ((rc = info->cfi_cmd_set->flash_write_cfibuffer (info, wp, src, i)) != ERR_OK)
			return rc;
		i -= i & (info->portwidth - 1);
		wp += i;
		src += i;
		cnt -= i;
	}
#else
	while (cnt >= info->portwidth) {
		cword = 0;
		for (i = 0; i < info->portwidth; i++) {
			flash_add_byte (info, &cword, *src++);
		}
		if ((rc = flash_write_cfiword (info, wp, cword)) != 0)
			return rc;
		wp += info->portwidth;
		cnt -= info->portwidth;
	}
#endif /* CONFIG_CFI_BUFFER_WRITE */
	if (cnt == 0) {
		return 0;
	}

	/*
	 * handle unaligned tail bytes
	 */
	cword = 0;
	p = (uchar*)wp;
	for (i = 0; (i < info->portwidth) && (cnt > 0); ++i) {
		flash_add_byte (info, &cword, *src++);
		--cnt;
	}
	for (; i < info->portwidth; ++i) {
		flash_add_byte (info, &cword, flash_read8(p + i));
	}

	return flash_write_cfiword (info, wp, cword);
}

static int flash_real_protect (struct flash_info *info, long sector, int prot)
{
	int retcode = 0;

	retcode = info->cfi_cmd_set->flash_real_protect(info, sector, prot);

	if (retcode)
		return retcode;

	if ((retcode =
	     flash_status_check (info, sector, info->erase_blk_tout,
				      prot ? "protect" : "unprotect")) == 0) {

		info->protect[sector] = prot;

		/*
		 * On some of Intel's flash chips (marked via legacy_unlock)
		 * unprotect unprotects all locking.
		 */
		if ((prot == 0) && (info->legacy_unlock)) {
			flash_sect_t i;

			for (i = 0; i < info->sector_count; i++) {
				if (info->protect[i])
					flash_real_protect (info, i, 1);
			}
		}
	}
	return retcode;
}

static int cfi_mtd_protect(struct flash_info *finfo, loff_t offset, size_t len, int prot)
{
	unsigned long start, end;
	int i, ret = 0;

	start = find_sector(finfo, (unsigned long)finfo->base + offset);
	end   = find_sector(finfo, (unsigned long)finfo->base + offset + len - 1);

	for (i = start; i <= end; i++) {
		ret = flash_real_protect (finfo, i, prot);
		if (ret)
			goto out;
	}
out:
	return ret;
}

static int cfi_mtd_lock(struct mtd_info *mtd, loff_t offset, size_t len)
{
	struct flash_info *finfo = container_of(mtd, struct flash_info, mtd);

	return cfi_mtd_protect(finfo, offset, len, 1);
}

static int cfi_mtd_unlock(struct mtd_info *mtd, loff_t offset, size_t len)
{
	struct flash_info *finfo = container_of(mtd, struct flash_info, mtd);

	return cfi_mtd_protect(finfo, offset, len, 0);
}

static void cfi_info (struct device_d* dev)
{
        struct flash_info *info = (struct flash_info *)dev->priv;
	int i;

	if (info->flash_id != FLASH_MAN_CFI) {
		puts ("missing or unknown FLASH type\n");
		return;
	}

	printf ("CFI conformant FLASH (%d x %d)",
		(info->portwidth << 3), (info->chipwidth << 3));
	printf ("  Size: %ld MB in %d Sectors\n",
		info->size >> 20, info->sector_count);
	printf ("  ");
	switch (info->vendor) {
		case CFI_CMDSET_INTEL_STANDARD:
			printf ("Intel Standard");
			break;
		case CFI_CMDSET_INTEL_EXTENDED:
			printf ("Intel Extended");
			break;
		case CFI_CMDSET_AMD_STANDARD:
			printf ("AMD Standard");
			break;
		case CFI_CMDSET_AMD_EXTENDED:
			printf ("AMD Extended");
			break;
		default:
			printf ("Unknown (%d)", info->vendor);
			break;
	}
	printf (" command set, Manufacturer ID: 0x%02X, Device ID: 0x%02X",
		info->manufacturer_id, info->device_id);
	if (info->device_id == 0x7E) {
		printf("%04X", info->device_id2);
	}
	printf ("\n  Erase timeout: %ld ms, write timeout: %ld us\n",
		info->erase_blk_tout,
		info->write_tout);
	if (info->buffer_size > 1) {
		printf ("  Buffer write timeout: %ld us, buffer size: %d bytes\n",
		info->buffer_write_tout,
		info->buffer_size);
	}

	puts ("\n  Sector Start Addresses:");
	for (i = 0; i < info->sector_count; ++i) {
		if ((i % 5) == 0)
			printf ("\n");
#ifdef CFG_FLASH_EMPTY_INFO
	{
		int k;
		int size;
		int erased;
		volatile unsigned long *flash;

		/*
		 * Check if whole sector is erased
		 */
		if (i != (info->sector_count - 1))
			size = info->start[i + 1] - info->start[i];
		else
			size = info->start[0] + info->size - info->start[i];
		erased = 1;
		flash = (volatile unsigned long *) info->start[i];
		size = size >> 2;	/* divide by 4 for longword access */
		for (k = 0; k < size; k++) {
			if (*flash++ != 0xffffffff) {
				erased = 0;
				break;
			}
		}

		/* print empty and read-only info */
		printf ("  %08lX %c %s ",
			info->start[i],
			erased ? 'E' : ' ',
			info->protect[i] ? "RO" : "  ");
	}
#else	/* ! CFG_FLASH_EMPTY_INFO */
		printf ("  %08lX   %s ",
			info->start[i],
			info->protect[i] ? "RO" : "  ");
#endif
	}
	putchar('\n');
	return;
}

#if 0
/*
 * flash_read_user_serial - read the OneTimeProgramming cells
 */
static void flash_read_user_serial (struct flash_info *info, void *buffer, int offset,
			     int len)
{
	uchar *src;
	uchar *dst;

	dst = buffer;
	src = flash_make_addr (info, 0, FLASH_OFFSET_USER_PROTECTION);
	flash_write_cmd (info, 0, 0, FLASH_CMD_READ_ID);
	memcpy (dst, src + offset, len);
	flash_write_cmd (info, 0, 0, info->cmd_reset);
}

/*
 * flash_read_factory_serial - read the device Id from the protection area
 */
static void flash_read_factory_serial (struct flash_info *info, void *buffer, int offset,
				int len)
{
	uchar *src;

	src = flash_make_addr (info, 0, FLASH_OFFSET_INTEL_PROTECTION);
	flash_write_cmd (info, 0, 0, FLASH_CMD_READ_ID);
	memcpy (buffer, src + offset, len);
	flash_write_cmd (info, 0, 0, info->cmd_reset);
}

#endif

int flash_status_check (struct flash_info *info, flash_sect_t sector,
			       uint64_t tout, char *prompt)
{
	return info->cfi_cmd_set->flash_status_check(info, sector, tout, prompt);
}

/*
 *  wait for XSR.7 to be set. Time out with an error if it does not.
 *  This routine does not set the flash to read-array mode.
 */
int flash_generic_status_check (struct flash_info *info, flash_sect_t sector,
			       uint64_t tout, char *prompt)
{
	uint64_t start;

        tout *= 1000000;

	/* Wait for command completion */
	start = get_time_ns();
	while (info->cfi_cmd_set->flash_is_busy (info, sector)) {
		if (is_timeout(start, tout)) {
			dev_err(info->dev, "Flash %s timeout at address %lx data %lx\n",
				prompt, info->start[sector],
				flash_read_long (info, sector, 0));
			flash_write_cmd (info, sector, 0, info->cmd_reset);
			return ERR_TIMOUT;
		}
		udelay (1);		/* also triggers watchdog */
	}
	return ERR_OK;
}

/*
 * make a proper sized command based on the port and chip widths
 */
void flash_make_cmd(struct flash_info *info, u32 cmd, cfiword_t *cmdbuf)
{
	cfiword_t result = 0;
	int i = info->portwidth / info->chipwidth;

	while (i--)
		result = (result << (8 * info->chipwidth)) | cmd;
	*cmdbuf = result;
}

/*
 * Write a proper sized command to the correct address
 */
void flash_write_cmd(struct flash_info *info, flash_sect_t sect,
				uint offset, u32 cmd)
{

	uchar *addr;
	cfiword_t cword;

	addr = flash_make_addr (info, sect, offset);
	flash_make_cmd (info, cmd, &cword);

	dev_dbg(info->dev, "%s: %p %lX %X => %p " CFI_WORD_FMT "\n", __func__,
			info, sect, offset, addr, cword);

	flash_write_word(info, cword, addr);
}

int flash_isequal(struct flash_info *info, flash_sect_t sect,
				uint offset, u32 cmd)
{
	void *addr;
	cfiword_t cword;
	int retval;

	addr = flash_make_addr (info, sect, offset);
	flash_make_cmd (info, cmd, &cword);

	dev_dbg(info->dev, "is= cmd %x(%c) addr %p ", cmd, cmd, addr);
	if (bankwidth_is_1(info)) {
		dev_dbg(info->dev, "is= %x %x\n", flash_read8(addr), (u8)cword);
		retval = (flash_read8(addr) == cword);
	} else if (bankwidth_is_2(info)) {
		dev_dbg(info->dev, "is= %4.4x %4.4x\n", flash_read16(addr), (u16)cword);
		retval = (flash_read16(addr) == cword);
	} else if (bankwidth_is_4(info)) {
		dev_dbg(info->dev, "is= %8.8x %8.8x\n", flash_read32(addr), (u32)cword);
		retval = (flash_read32(addr) == cword);
	} else if (bankwidth_is_8(info)) {
#ifdef DEBUG
		{
			char str1[20];
			char str2[20];

			print_longlong (str1, flash_read32(addr));
			print_longlong (str2, cword);
			dev_dbg(info->dev, "is= %s %s\n", str1, str2);
		}
#endif
		retval = (flash_read64(addr) == cword);
	} else
		retval = 0;

	return retval;
}

int flash_isset(struct flash_info *info, flash_sect_t sect,
				uint offset, u32 cmd)
{
	void *addr = flash_make_addr (info, sect, offset);
	cfiword_t cword;
	int retval;

	flash_make_cmd (info, cmd, &cword);
	if (bankwidth_is_1(info)) {
		retval = ((flash_read8(addr) & cword) == cword);
	} else if (bankwidth_is_2(info)) {
		retval = ((flash_read16(addr) & cword) == cword);
	} else if (bankwidth_is_4(info)) {
		retval = ((flash_read32(addr) & cword) == cword);
	} else if (bankwidth_is_8(info)) {
		retval = ((flash_read64(addr) & cword) == cword);
	} else
		retval = 0;

	return retval;
}

static int cfi_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	struct flash_info *info = container_of(mtd, struct flash_info, mtd);

	memcpy(buf, info->base + from, len);
	*retlen = len;

	return 0;
}

static int cfi_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct flash_info *info = container_of(mtd, struct flash_info, mtd);
	int ret;

	ret = write_buff(info, buf, (unsigned long)info->base + to, len);
	*retlen = len;

        return ret;
}

static int cfi_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct flash_info *info = container_of(mtd, struct flash_info, mtd);
	int ret;

	ret = cfi_erase(info, instr->len, instr->addr);

	if (ret) {
		instr->state = MTD_ERASE_FAILED;
		return -EIO;
	}

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static void cfi_init_mtd(struct flash_info *info)
{
	struct mtd_info *mtd = &info->mtd;

	mtd->read = cfi_mtd_read;
	mtd->write = cfi_mtd_write;
	mtd->erase = cfi_mtd_erase;
	mtd->lock = cfi_mtd_lock;
	mtd->unlock = cfi_mtd_unlock;
	mtd->size = info->size;
	mtd->erasesize = info->eraseregions[1].erasesize; /* FIXME */
	mtd->writesize = 1;
	mtd->subpage_sft = 0;
	mtd->eraseregions = info->eraseregions;
	mtd->numeraseregions = info->numeraseregions;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->type = MTD_NORFLASH;
	mtd->parent = info->dev;

	add_mtd_device(mtd, "nor");
}

static int cfi_probe (struct device_d *dev)
{
	struct flash_info *info = xzalloc(sizeof(*info));

	dev->priv = (void *)info;

	/* Init: no FLASHes known */
	info->flash_id = FLASH_UNKNOWN;
	info->cmd_reset = FLASH_CMD_RESET;
	info->base = dev_request_mem_region(dev, 0);
	info->size = flash_get_size(info);
	info->dev = dev;

	if (info->flash_id == FLASH_UNKNOWN) {
		dev_warn(dev, "## Unknown FLASH on Bank at 0x%08x - Size = 0x%08lx = %ld MB\n",
			dev->resource[0].start, info->size, info->size << 20);
		return -ENODEV;
	}

	dev_info(dev, "found cfi flash at %p, size %ld\n",
			info->base, info->size);

	dev->info = cfi_info;

	cfi_init_mtd(info);

	return 0;
}

static __maybe_unused struct of_device_id cfi_dt_ids[] = {
	{
		.compatible = "cfi-flash",
	}, {
		/* sentinel */
	}
};

static struct driver_d cfi_driver = {
	.name    = "cfi_flash",
	.probe   = cfi_probe,
	.of_compatible = DRV_OF_COMPAT(cfi_dt_ids),
};
device_platform_driver(cfi_driver);
