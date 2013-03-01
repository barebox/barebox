/*
 * Handles the M-Systems DiskOnChip G3 chip
 *
 * Copyright (C) 2011 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Taken from linux kernel.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <errno.h>
#include <malloc.h>
#include <clock.h>
#include <io.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/err.h>
#include <linux/bitrev.h>
#include <linux/bch.h>

#include "docg3.h"

static unsigned reliable_mode;

/**
 * struct docg3_bch - BCH engine
 */
static struct bch_control *docg3_bch;
struct mtd_info *docg3_floors[DOC_MAX_NBFLOORS];

static inline u8 doc_readb(struct docg3 *docg3, u16 reg)
{
	u8 val = readb(docg3->base + reg);

	doc_dbg("readb(%04x) -> %02x\n", reg, val);
	return val;
}

static inline u16 doc_readw(struct docg3 *docg3, u16 reg)
{
	u16 val = readw(docg3->base + reg);

	doc_dbg("readb(%04x) -> %04x\n", reg, val);
	return val;
}

static inline void doc_writeb(struct docg3 *docg3, u8 val, u16 reg)
{
	doc_dbg("writeb(%02x into %04x)\n", val, reg);
	writeb(val, docg3->base + reg);
}

static inline void doc_writew(struct docg3 *docg3, u16 val, u16 reg)
{
	doc_dbg("writew(%04x into %04x)\n", val, reg);
	writew(val, docg3->base + reg);
}

static inline void doc_flash_command(struct docg3 *docg3, u8 cmd)
{
	doc_writeb(docg3, cmd, DOC_FLASHCOMMAND);
}

static inline void doc_flash_sequence(struct docg3 *docg3, u8 seq)
{
	doc_writeb(docg3, seq, DOC_FLASHSEQUENCE);
}

static inline void doc_flash_address(struct docg3 *docg3, u8 addr)
{
	doc_writeb(docg3, addr, DOC_FLASHADDRESS);
}

static int doc_register_readb(struct docg3 *docg3, int reg)
{
	u8 val;

	doc_writew(docg3, reg, DOC_READADDRESS);
	val = doc_readb(docg3, reg);
	doc_vdbg("Read register %04x : %02x\n", reg, val);
	return val;
}

static int doc_register_readw(struct docg3 *docg3, int reg)
{
	u16 val;

	doc_writew(docg3, reg, DOC_READADDRESS);
	val = doc_readw(docg3, reg);
	doc_vdbg("Read register %04x : %04x\n", reg, val);
	return val;
}

static void doc_delay(struct docg3 *docg3, int nbNOPs)
{
	int i;

	doc_vdbg("NOP x %d\n", nbNOPs);
	for (i = 0; i < nbNOPs; i++)
		doc_writeb(docg3, 0, DOC_NOP);
}

static int is_prot_seq_error(struct docg3 *docg3)
{
	int ctrl;

	ctrl = doc_register_readb(docg3, DOC_FLASHCONTROL);
	return ctrl & (DOC_CTRL_PROTECTION_ERROR | DOC_CTRL_SEQUENCE_ERROR);
}

static int doc_is_ready(struct docg3 *docg3)
{
	int ctrl;

	ctrl = doc_register_readb(docg3, DOC_FLASHCONTROL);
	return ctrl & DOC_CTRL_FLASHREADY;
}

static int doc_wait_ready(struct docg3 *docg3)
{
	int maxWaitCycles = 100;

	do {
		doc_delay(docg3, 4);
	} while (!doc_is_ready(docg3) && maxWaitCycles--);
	doc_delay(docg3, 2);
	if (maxWaitCycles > 0)
		return 0;
	else
		return -EIO;
}

static int doc_reset_seq(struct docg3 *docg3)
{
	int ret;

	doc_writeb(docg3, 0x10, DOC_FLASHCONTROL);
	doc_flash_sequence(docg3, DOC_SEQ_RESET);
	doc_flash_command(docg3, DOC_CMD_RESET);
	doc_delay(docg3, 2);
	ret = doc_wait_ready(docg3);

	doc_dbg("doc_reset_seq() -> isReady=%s\n", ret ? "false" : "true");
	return ret;
}

static void doc_read_data_area(struct docg3 *docg3, void *buf, int len,
			       int first)
{
	int i, cdr, len4;
	u16 data16, *dst16;
	u8 data8, *dst8;

	doc_dbg("doc_read_data_area(buf=%p, len=%d)\n", buf, len);
	cdr = len & 0x1;
	len4 = len - cdr;

	if (first)
		doc_writew(docg3, DOC_IOSPACE_DATA, DOC_READADDRESS);
	dst16 = buf;
	for (i = 0; i < len4; i += 2) {
		data16 = doc_readw(docg3, DOC_IOSPACE_DATA);
		if (dst16) {
			*dst16 = data16;
			dst16++;
		}
	}

	if (cdr) {
		doc_writew(docg3, DOC_IOSPACE_DATA | DOC_READADDR_ONE_BYTE,
			   DOC_READADDRESS);
		doc_delay(docg3, 1);
		dst8 = (u8 *)dst16;
		for (i = 0; i < cdr; i++) {
			data8 = doc_readb(docg3, DOC_IOSPACE_DATA);
			if (dst8) {
				*dst8 = data8;
				dst8++;
			}
		}
	}
}

static void doc_set_reliable_mode(struct docg3 *docg3)
{
	static char *strmode[] = { "normal", "fast", "reliable", "invalid" };

	doc_dbg("doc_set_reliable_mode(%s)\n", strmode[docg3->reliable]);
	switch (docg3->reliable) {
	case 0:
		break;
	case 1:
		doc_flash_sequence(docg3, DOC_SEQ_SET_FASTMODE);
		doc_flash_command(docg3, DOC_CMD_FAST_MODE);
		break;
	case 2:
		doc_flash_sequence(docg3, DOC_SEQ_SET_RELIABLEMODE);
		doc_flash_command(docg3, DOC_CMD_FAST_MODE);
		doc_flash_command(docg3, DOC_CMD_RELIABLE_MODE);
		break;
	default:
		doc_err("doc_set_reliable_mode(): invalid mode\n");
		break;
	}
	doc_delay(docg3, 2);
}

static void doc_set_asic_mode(struct docg3 *docg3, u8 mode)
{
	int i;

	for (i = 0; i < 12; i++)
		doc_readb(docg3, DOC_IOSPACE_IPL);

	mode |= DOC_ASICMODE_MDWREN;
	doc_dbg("doc_set_asic_mode(%02x)\n", mode);
	doc_writeb(docg3, mode, DOC_ASICMODE);
	doc_writeb(docg3, ~mode, DOC_ASICMODECONFIRM);
	doc_delay(docg3, 1);
}

static void doc_set_device_id(struct docg3 *docg3, int id)
{
	u8 ctrl;

	doc_dbg("doc_set_device_id(%d)\n", id);
	doc_writeb(docg3, id, DOC_DEVICESELECT);
	ctrl = doc_register_readb(docg3, DOC_FLASHCONTROL);

	ctrl &= ~DOC_CTRL_VIOLATION;
	ctrl |= DOC_CTRL_CE;
	doc_writeb(docg3, ctrl, DOC_FLASHCONTROL);
}

static int doc_set_extra_page_mode(struct docg3 *docg3)
{
	int fctrl;

	doc_dbg("doc_set_extra_page_mode()\n");
	doc_flash_sequence(docg3, DOC_SEQ_PAGE_SIZE_532);
	doc_flash_command(docg3, DOC_CMD_PAGE_SIZE_532);
	doc_delay(docg3, 2);

	fctrl = doc_register_readb(docg3, DOC_FLASHCONTROL);
	if (fctrl & (DOC_CTRL_PROTECTION_ERROR | DOC_CTRL_SEQUENCE_ERROR))
		return -EIO;
	else
		return 0;
}

static void doc_setup_addr_sector(struct docg3 *docg3, int sector)
{
	doc_delay(docg3, 1);
	doc_flash_address(docg3, sector & 0xff);
	doc_flash_address(docg3, (sector >> 8) & 0xff);
	doc_flash_address(docg3, (sector >> 16) & 0xff);
	doc_delay(docg3, 1);
}

static int doc_read_seek(struct docg3 *docg3, int block0, int block1, int page,
			 int wear, int ofs)
{
	int sector, ret = 0;

	doc_dbg("doc_seek(blocks=(%d,%d), page=%d, ofs=%d, wear=%d)\n",
		block0, block1, page, ofs, wear);

	if (!wear && (ofs < 2 * DOC_LAYOUT_PAGE_SIZE)) {
		doc_flash_sequence(docg3, DOC_SEQ_SET_PLANE1);
		doc_flash_command(docg3, DOC_CMD_READ_PLANE1);
		doc_delay(docg3, 2);
	} else {
		doc_flash_sequence(docg3, DOC_SEQ_SET_PLANE2);
		doc_flash_command(docg3, DOC_CMD_READ_PLANE2);
		doc_delay(docg3, 2);
	}

	doc_set_reliable_mode(docg3);
	if (wear)
		ret = doc_set_extra_page_mode(docg3);
	if (ret)
		goto out;

	doc_flash_sequence(docg3, DOC_SEQ_READ);
	sector = (block0 << DOC_ADDR_BLOCK_SHIFT) + (page & DOC_ADDR_PAGE_MASK);
	doc_flash_command(docg3, DOC_CMD_PROG_BLOCK_ADDR);
	doc_setup_addr_sector(docg3, sector);

	sector = (block1 << DOC_ADDR_BLOCK_SHIFT) + (page & DOC_ADDR_PAGE_MASK);
	doc_flash_command(docg3, DOC_CMD_PROG_BLOCK_ADDR);
	doc_setup_addr_sector(docg3, sector);
	doc_delay(docg3, 1);

out:
	return ret;
}

static int doc_read_page_ecc_init(struct docg3 *docg3, int len)
{
	doc_writew(docg3, DOC_ECCCONF0_READ_MODE
		   | DOC_ECCCONF0_BCH_ENABLE | DOC_ECCCONF0_HAMMING_ENABLE
		   | (len & DOC_ECCCONF0_DATA_BYTES_MASK),
		   DOC_ECCCONF0);
	doc_delay(docg3, 4);
	doc_register_readb(docg3, DOC_FLASHCONTROL);
	return doc_wait_ready(docg3);
}

static void doc_hamming_ecc_init(struct docg3 *docg3, int nb_bytes)
{
	u8 ecc_conf1;

	ecc_conf1 = doc_register_readb(docg3, DOC_ECCCONF1);
	ecc_conf1 &= ~DOC_ECCCONF1_HAMMING_BITS_MASK;
	ecc_conf1 |= (nb_bytes & DOC_ECCCONF1_HAMMING_BITS_MASK);
	doc_writeb(docg3, ecc_conf1, DOC_ECCCONF1);
}

static int doc_ecc_bch_fix_data(struct docg3 *docg3, void *buf, u8 *hwecc)
{
	u8 ecc[DOC_ECC_BCH_SIZE];
	int errorpos[DOC_ECC_BCH_T], i, numerrs;

	for (i = 0; i < DOC_ECC_BCH_SIZE; i++)
		ecc[i] = bitrev8(hwecc[i]);
	numerrs = decode_bch(docg3_bch, NULL, DOC_ECC_BCH_COVERED_BYTES,
			     NULL, ecc, NULL, errorpos);
	BUG_ON(numerrs == -EINVAL);
	if (numerrs < 0)
		goto out;

	for (i = 0; i < numerrs; i++)
		errorpos[i] = (errorpos[i] & ~7) | (7 - (errorpos[i] & 7));
	for (i = 0; i < numerrs; i++)
		if (errorpos[i] < DOC_ECC_BCH_COVERED_BYTES*8)
			/* error is located in data, correct it */
			change_bit(errorpos[i], buf);
out:
	doc_dbg("doc_ecc_bch_fix_data: flipped %d bits\n", numerrs);
	return numerrs;
}

static int doc_read_page_prepare(struct docg3 *docg3, int block0, int block1,
				 int page, int offset)
{
	int wear_area = 0, ret = 0;

	doc_dbg("doc_read_page_prepare(blocks=(%d,%d), page=%d, ofsInPage=%d)\n",
		block0, block1, page, offset);
	if (offset >= DOC_LAYOUT_WEAR_OFFSET)
		wear_area = 1;
	if (!wear_area && offset > (DOC_LAYOUT_PAGE_OOB_SIZE * 2))
		return -EINVAL;

	doc_set_device_id(docg3, docg3->device_id);
	ret = doc_reset_seq(docg3);
	if (ret)
		goto err;

	/* Program the flash address block and page */
	ret = doc_read_seek(docg3, block0, block1, page, wear_area, offset);
	if (ret)
		goto err;

	doc_flash_command(docg3, DOC_CMD_READ_ALL_PLANES);
	doc_delay(docg3, 2);
	doc_wait_ready(docg3);

	doc_flash_command(docg3, DOC_CMD_SET_ADDR_READ);
	doc_delay(docg3, 1);
	if (offset >= DOC_LAYOUT_PAGE_SIZE * 2)
		offset -= 2 * DOC_LAYOUT_PAGE_SIZE;
	doc_flash_address(docg3, offset >> 2);
	doc_delay(docg3, 1);
	doc_wait_ready(docg3);

	doc_flash_command(docg3, DOC_CMD_READ_FLASH);

	return 0;
err:
	doc_writeb(docg3, 0, DOC_DATAEND);
	doc_delay(docg3, 2);
	return -EIO;
}

static int doc_read_page_getbytes(struct docg3 *docg3, int len, u_char *buf,
				  int first, int last_odd)
{
	if (last_odd && len > 0) {
		doc_read_data_area(docg3, buf, 1, first);
		doc_read_data_area(docg3, buf ? buf + 1 : buf, len - 1, 0);
	} else {
		doc_read_data_area(docg3, buf, len, first);
	}
	doc_delay(docg3, 2);
	return len;
}

static void doc_get_bch_hw_ecc(struct docg3 *docg3, u8 *hwecc)
{
	int i;

	for (i = 0; i < DOC_ECC_BCH_SIZE; i++)
		hwecc[i] = doc_register_readb(docg3, DOC_BCH_HW_ECC(i));
}

static void doc_page_finish(struct docg3 *docg3)
{
	doc_writeb(docg3, 0, DOC_DATAEND);
	doc_delay(docg3, 2);
}

static void doc_read_page_finish(struct docg3 *docg3)
{
	doc_page_finish(docg3);
	doc_set_device_id(docg3, 0);
}

static void calc_block_sector(loff_t from, int *block0, int *block1, int *page,
			      int *ofs, int reliable)
{
	uint sector, pages_biblock;

	pages_biblock = DOC_LAYOUT_PAGES_PER_BLOCK * DOC_LAYOUT_NBPLANES;
	if (reliable == 1 || reliable == 2)
		pages_biblock /= 2;

	sector = from / DOC_LAYOUT_PAGE_SIZE;
	*block0 = sector / pages_biblock * DOC_LAYOUT_NBPLANES;
	*block1 = *block0 + 1;
	*page = sector % pages_biblock;
	*page /= DOC_LAYOUT_NBPLANES;
	if (reliable == 1 || reliable == 2)
		*page *= 2;
	if (sector % 2)
		*ofs = DOC_LAYOUT_PAGE_OOB_SIZE;
	else
		*ofs = 0;
}

static int doc_read_oob(struct mtd_info *mtd, loff_t from,
			struct mtd_oob_ops *ops)
{
	struct docg3 *docg3 = mtd->priv;
	int block0, block1, page, ret, skip, ofs = 0;
	u8 *oobbuf = ops->oobbuf;
	u8 *buf = ops->datbuf;
	size_t len, ooblen, nbdata, nboob;
	u8 hwecc[DOC_ECC_BCH_SIZE], eccconf1;

	if (buf)
		len = ops->len;
	else
		len = 0;
	if (oobbuf)
		ooblen = ops->ooblen;
	else
		ooblen = 0;

	if (oobbuf && ops->mode == MTD_OPS_PLACE_OOB)
		oobbuf += ops->ooboffs;

	doc_dbg("doc_read_oob(from=%lld, mode=%d, data=(%p:%zu), oob=(%p:%zu))\n",
		from, ops->mode, buf, len, oobbuf, ooblen);
	if (ooblen % DOC_LAYOUT_OOB_SIZE)
		return -EINVAL;

	ret = -EINVAL;
	calc_block_sector(from + len, &block0, &block1, &page, &ofs,
			  docg3->reliable);
	if (block1 > docg3->max_block)
		goto err;

	ops->oobretlen = 0;
	ops->retlen = 0;
	ret = 0;
	skip = from % DOC_LAYOUT_PAGE_SIZE;
	while (!ret && (len > 0 || ooblen > 0)) {
		calc_block_sector(from, &block0, &block1, &page, &ofs,
			docg3->reliable);
		nbdata = min_t(size_t, len, DOC_LAYOUT_PAGE_SIZE - skip);
		nboob = min_t(size_t, ooblen, (size_t)DOC_LAYOUT_OOB_SIZE);
		ret = doc_read_page_prepare(docg3, block0, block1, page, ofs);
		if (ret < 0)
			goto err;
		ret = doc_read_page_ecc_init(docg3, DOC_ECC_BCH_TOTAL_BYTES);
		if (ret < 0)
			goto err_in_read;
		ret = doc_read_page_getbytes(docg3, skip, NULL, 1, 0);
		if (ret < skip)
			goto err_in_read;
		ret = doc_read_page_getbytes(docg3, nbdata, buf, 0, skip % 2);
		if (ret < nbdata)
			goto err_in_read;
		doc_read_page_getbytes(docg3,
				       DOC_LAYOUT_PAGE_SIZE - nbdata - skip,
				       NULL, 0, (skip + nbdata) % 2);
		ret = doc_read_page_getbytes(docg3, nboob, oobbuf, 0, 0);
		if (ret < nboob)
			goto err_in_read;
		doc_read_page_getbytes(docg3, DOC_LAYOUT_OOB_SIZE - nboob,
				       NULL, 0, nboob % 2);

		doc_get_bch_hw_ecc(docg3, hwecc);
		eccconf1 = doc_register_readb(docg3, DOC_ECCCONF1);

		if (nboob >= DOC_LAYOUT_OOB_SIZE) {
			doc_dbg("OOB - INFO: %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
				oobbuf[0], oobbuf[1], oobbuf[2], oobbuf[3],
				oobbuf[4], oobbuf[5], oobbuf[6]);
			doc_dbg("OOB - HAMMING: %02x\n", oobbuf[7]);
			doc_dbg("OOB - BCH_ECC: %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
				oobbuf[8], oobbuf[9], oobbuf[10], oobbuf[11],
				oobbuf[12], oobbuf[13], oobbuf[14]);
			doc_dbg("OOB - UNUSED: %02x\n", oobbuf[15]);
		}
		doc_dbg("ECC checks: ECCConf1=%x\n", eccconf1);
		doc_dbg("ECC HW_ECC: %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			hwecc[0], hwecc[1], hwecc[2], hwecc[3], hwecc[4],
			hwecc[5], hwecc[6]);

		ret = -EIO;
		if (is_prot_seq_error(docg3))
			goto err_in_read;
		ret = 0;
		if ((block0 >= DOC_LAYOUT_BLOCK_FIRST_DATA) &&
		    (eccconf1 & DOC_ECCCONF1_BCH_SYNDROM_ERR) &&
		    (eccconf1 & DOC_ECCCONF1_PAGE_IS_WRITTEN) &&
		    (ops->mode != MTD_OPS_RAW) &&
		    (nbdata == DOC_LAYOUT_PAGE_SIZE)) {
			ret = doc_ecc_bch_fix_data(docg3, buf, hwecc);
			if (ret < 0) {
				mtd->ecc_stats.failed++;
				ret = -EBADMSG;
			}
			if (ret > 0) {
				mtd->ecc_stats.corrected += ret;
				ret = -EUCLEAN;
			}
		}

		doc_read_page_finish(docg3);
		ops->retlen += nbdata;
		ops->oobretlen += nboob;
		buf += nbdata;
		oobbuf += nboob;
		len -= nbdata;
		ooblen -= nboob;
		from += DOC_LAYOUT_PAGE_SIZE;
		skip = 0;
	}

	return ret;
err_in_read:
	doc_read_page_finish(docg3);
err:
	return ret;
}

static int doc_read(struct mtd_info *mtd, loff_t from, size_t len,
	     size_t *retlen, u_char *buf)
{
	struct mtd_oob_ops ops;
	size_t ret;

	memset(&ops, 0, sizeof(ops));
	ops.datbuf = buf;
	ops.len = len;
	ops.mode = MTD_OPS_AUTO_OOB;

	ret = doc_read_oob(mtd, from, &ops);
	*retlen = ops.retlen;
	return ret;
}

static int doc_reload_bbt(struct docg3 *docg3)
{
	int block = DOC_LAYOUT_BLOCK_BBT;
	int ret = 0, nbpages, page;
	u_char *buf = docg3->bbt;

	nbpages = DIV_ROUND_UP(docg3->max_block + 1, 8 * DOC_LAYOUT_PAGE_SIZE);
	for (page = 0; !ret && (page < nbpages); page++) {
		ret = doc_read_page_prepare(docg3, block, block + 1,
					    page + DOC_LAYOUT_PAGE_BBT, 0);
		if (!ret)
			ret = doc_read_page_ecc_init(docg3,
						     DOC_LAYOUT_PAGE_SIZE);
		if (!ret)
			doc_read_page_getbytes(docg3, DOC_LAYOUT_PAGE_SIZE,
					       buf, 1, 0);
		buf += DOC_LAYOUT_PAGE_SIZE;
	}
	doc_read_page_finish(docg3);
	return ret;
}

static int doc_block_isbad(struct mtd_info *mtd, loff_t from)
{
	struct docg3 *docg3 = mtd->priv;
	int block0, block1, page, ofs, is_good;

	calc_block_sector(from, &block0, &block1, &page, &ofs,
		docg3->reliable);
	doc_dbg("doc_block_isbad(from=%lld) => block=(%d,%d), page=%d, ofs=%d\n",
		from, block0, block1, page, ofs);

	if (block0 < DOC_LAYOUT_BLOCK_FIRST_DATA)
		return 0;
	if (block1 > docg3->max_block)
		return -EINVAL;

	is_good = docg3->bbt[block0 >> 3] & (1 << (block0 & 0x7));
	return !is_good;
}

#ifdef CONFIG_MTD_WRITE
static int doc_guess_autoecc(struct mtd_oob_ops *ops)
{
	int autoecc;

	switch (ops->mode) {
	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_AUTO_OOB:
		autoecc = 1;
		break;
	case MTD_OPS_RAW:
		autoecc = 0;
		break;
	default:
		autoecc = -EINVAL;
	}
	return autoecc;
}

static void doc_fill_autooob(u8 *dst, u8 *oobsrc)
{
	memcpy(dst, oobsrc, DOC_LAYOUT_OOB_PAGEINFO_SZ);
	dst[DOC_LAYOUT_OOB_UNUSED_OFS] = oobsrc[DOC_LAYOUT_OOB_PAGEINFO_SZ];
}

static int doc_backup_oob(struct docg3 *docg3, loff_t to,
			  struct mtd_oob_ops *ops)
{
	int ooblen = ops->ooblen, autoecc;

	if (ooblen != DOC_LAYOUT_OOB_SIZE)
		return -EINVAL;
	autoecc = doc_guess_autoecc(ops);
	if (autoecc < 0)
		return autoecc;

	docg3->oob_write_ofs = to;
	docg3->oob_autoecc = autoecc;
	if (ops->mode == MTD_OPS_AUTO_OOB) {
		doc_fill_autooob(docg3->oob_write_buf, ops->oobbuf);
		ops->oobretlen = 8;
	} else {
		memcpy(docg3->oob_write_buf, ops->oobbuf, DOC_LAYOUT_OOB_SIZE);
		ops->oobretlen = DOC_LAYOUT_OOB_SIZE;
	}
	return 0;
}

static void doc_write_data_area(struct docg3 *docg3, const void *buf, int len)
{
	int i, cdr, len4;
	u16 *src16;
	u8 *src8;

	doc_dbg("doc_write_data_area(buf=%p, len=%d)\n", buf, len);
	cdr = len & 0x3;
	len4 = len - cdr;

	doc_writew(docg3, DOC_IOSPACE_DATA, DOC_READADDRESS);
	src16 = (u16 *)buf;
	for (i = 0; i < len4; i += 2) {
		doc_writew(docg3, *src16, DOC_IOSPACE_DATA);
		src16++;
	}

	src8 = (u8 *)src16;
	for (i = 0; i < cdr; i++) {
		doc_writew(docg3, DOC_IOSPACE_DATA | DOC_READADDR_ONE_BYTE,
			   DOC_READADDRESS);
		doc_writeb(docg3, *src8, DOC_IOSPACE_DATA);
		src8++;
	}
}

static void doc_setup_writeaddr_sector(struct docg3 *docg3, int sector, int ofs)
{
	ofs = ofs >> 2;
	doc_delay(docg3, 1);
	doc_flash_address(docg3, ofs & 0xff);
	doc_flash_address(docg3, sector & 0xff);
	doc_flash_address(docg3, (sector >> 8) & 0xff);
	doc_flash_address(docg3, (sector >> 16) & 0xff);
	doc_delay(docg3, 1);
}

static int doc_write_seek(struct docg3 *docg3, int block0, int block1, int page,
			 int ofs)
{
	int ret = 0, sector;

	doc_dbg("doc_write_seek(blocks=(%d,%d), page=%d, ofs=%d)\n",
		block0, block1, page, ofs);

	doc_set_reliable_mode(docg3);

	if (ofs < 2 * DOC_LAYOUT_PAGE_SIZE) {
		doc_flash_sequence(docg3, DOC_SEQ_SET_PLANE1);
		doc_flash_command(docg3, DOC_CMD_READ_PLANE1);
		doc_delay(docg3, 2);
	} else {
		doc_flash_sequence(docg3, DOC_SEQ_SET_PLANE2);
		doc_flash_command(docg3, DOC_CMD_READ_PLANE2);
		doc_delay(docg3, 2);
	}

	doc_flash_sequence(docg3, DOC_SEQ_PAGE_SETUP);
	doc_flash_command(docg3, DOC_CMD_PROG_CYCLE1);

	sector = (block0 << DOC_ADDR_BLOCK_SHIFT) + (page & DOC_ADDR_PAGE_MASK);
	doc_setup_writeaddr_sector(docg3, sector, ofs);

	doc_flash_command(docg3, DOC_CMD_PROG_CYCLE3);
	doc_delay(docg3, 2);
	ret = doc_wait_ready(docg3);
	if (ret)
		goto out;

	doc_flash_command(docg3, DOC_CMD_PROG_CYCLE1);
	sector = (block1 << DOC_ADDR_BLOCK_SHIFT) + (page & DOC_ADDR_PAGE_MASK);
	doc_setup_writeaddr_sector(docg3, sector, ofs);
	doc_delay(docg3, 1);

out:
	return ret;
}

static int doc_write_page_ecc_init(struct docg3 *docg3, int len)
{
	doc_writew(docg3, DOC_ECCCONF0_WRITE_MODE
		   | DOC_ECCCONF0_BCH_ENABLE | DOC_ECCCONF0_HAMMING_ENABLE
		   | (len & DOC_ECCCONF0_DATA_BYTES_MASK),
		   DOC_ECCCONF0);
	doc_delay(docg3, 4);
	doc_register_readb(docg3, DOC_FLASHCONTROL);
	return doc_wait_ready(docg3);
}

static void doc_write_page_putbytes(struct docg3 *docg3, int len,
				    const u_char *buf)
{
	doc_write_data_area(docg3, buf, len);
	doc_delay(docg3, 2);
}

static void doc_ecc_disable(struct docg3 *docg3)
{
	doc_writew(docg3, DOC_ECCCONF0_READ_MODE, DOC_ECCCONF0);
	doc_delay(docg3, 4);
}

static int doc_get_op_status(struct docg3 *docg3)
{
	u8 status;

	doc_flash_sequence(docg3, DOC_SEQ_PLANES_STATUS);
	doc_flash_command(docg3, DOC_CMD_PLANES_STATUS);
	doc_delay(docg3, 5);

	doc_ecc_disable(docg3);
	doc_read_data_area(docg3, &status, 1, 1);
	return status;
}

static int doc_write_erase_wait_status(struct docg3 *docg3)
{
	int status, ret = 0;
	uint64_t start = get_time_ns();

	while (!is_timeout(start, 3000 * MSECOND) && !doc_is_ready(docg3));
	if (!doc_is_ready(docg3)) {
		doc_dbg("Timeout reached and the chip is still not ready\n");
		ret = -EAGAIN;
		goto out;
	}

	status = doc_get_op_status(docg3);
	if (status & DOC_PLANES_STATUS_FAIL) {
		doc_dbg("Erase/Write failed on (a) plane(s), status = %x\n",
			status);
		ret = -EIO;
	}

out:
	doc_page_finish(docg3);
	return ret;
}

static int doc_erase_block(struct docg3 *docg3, int block0, int block1)
{
	int ret, sector;

	doc_dbg("doc_erase_block(blocks=(%d,%d))\n", block0, block1);
	ret = doc_reset_seq(docg3);
	if (ret)
		return -EIO;

	doc_set_reliable_mode(docg3);
	doc_flash_sequence(docg3, DOC_SEQ_ERASE);

	sector = block0 << DOC_ADDR_BLOCK_SHIFT;
	doc_flash_command(docg3, DOC_CMD_PROG_BLOCK_ADDR);
	doc_setup_addr_sector(docg3, sector);
	sector = block1 << DOC_ADDR_BLOCK_SHIFT;
	doc_flash_command(docg3, DOC_CMD_PROG_BLOCK_ADDR);
	doc_setup_addr_sector(docg3, sector);
	doc_delay(docg3, 1);

	doc_flash_command(docg3, DOC_CMD_ERASECYCLE2);
	doc_delay(docg3, 2);

	if (is_prot_seq_error(docg3)) {
		doc_err("Erase blocks %d,%d error\n", block0, block1);
		return -EIO;
	}

	return doc_write_erase_wait_status(docg3);
}

static int doc_erase(struct mtd_info *mtd, struct erase_info *info)
{
	struct docg3 *docg3 = mtd->priv;
	uint64_t len;
	int block0, block1, page, ret, ofs = 0;

	doc_dbg("doc_erase(from=%d, len=%d\n", info->addr, info->len);
	doc_set_device_id(docg3, docg3->device_id);

	info->state = MTD_ERASE_PENDING;
	calc_block_sector(info->addr + info->len, &block0, &block1, &page,
			  &ofs, docg3->reliable);
	ret = -EINVAL;
	if (block1 > docg3->max_block || page || ofs)
		goto reset_err;

	ret = 0;
	calc_block_sector(info->addr, &block0, &block1, &page, &ofs,
			  docg3->reliable);
	doc_set_reliable_mode(docg3);
	for (len = info->len; !ret && len > 0; len -= mtd->erasesize) {
		info->state = MTD_ERASING;
		ret = doc_erase_block(docg3, block0, block1);
		block0 += 2;
		block1 += 2;
	}

	if (ret)
		goto reset_err;

	info->state = MTD_ERASE_DONE;
	return 0;

reset_err:
	info->state = MTD_ERASE_FAILED;
	return ret;
}

static int doc_write_page(struct docg3 *docg3, loff_t to, const u_char *buf,
			  const u_char *oob, int autoecc)
{
	int block0, block1, page, ret, ofs = 0;
	u8 hwecc[DOC_ECC_BCH_SIZE], hamming;

	doc_dbg("doc_write_page(to=%lld)\n", to);
	calc_block_sector(to, &block0, &block1, &page, &ofs, docg3->reliable);

	doc_set_device_id(docg3, docg3->device_id);
	ret = doc_reset_seq(docg3);
	if (ret)
		goto err;

	/* Program the flash address block and page */
	ret = doc_write_seek(docg3, block0, block1, page, ofs);
	if (ret)
		goto err;

	doc_write_page_ecc_init(docg3, DOC_ECC_BCH_TOTAL_BYTES);
	doc_delay(docg3, 2);
	doc_write_page_putbytes(docg3, DOC_LAYOUT_PAGE_SIZE, buf);

	if (oob && autoecc) {
		doc_write_page_putbytes(docg3, DOC_LAYOUT_OOB_PAGEINFO_SZ, oob);
		doc_delay(docg3, 2);
		oob += DOC_LAYOUT_OOB_UNUSED_OFS;

		hamming = doc_register_readb(docg3, DOC_HAMMINGPARITY);
		doc_delay(docg3, 2);
		doc_write_page_putbytes(docg3, DOC_LAYOUT_OOB_HAMMING_SZ,
					&hamming);
		doc_delay(docg3, 2);

		doc_get_bch_hw_ecc(docg3, hwecc);
		doc_write_page_putbytes(docg3, DOC_LAYOUT_OOB_BCH_SZ, hwecc);
		doc_delay(docg3, 2);

		doc_write_page_putbytes(docg3, DOC_LAYOUT_OOB_UNUSED_SZ, oob);
	}
	if (oob && !autoecc)
		doc_write_page_putbytes(docg3, DOC_LAYOUT_OOB_SIZE, oob);

	doc_delay(docg3, 2);
	doc_page_finish(docg3);
	doc_delay(docg3, 2);
	doc_flash_command(docg3, DOC_CMD_PROG_CYCLE2);
	doc_delay(docg3, 2);

	/*
	 * The wait status will perform another doc_page_finish() call, but that
	 * seems to please the docg3, so leave it.
	 */
	ret = doc_write_erase_wait_status(docg3);
	return ret;
err:
	doc_read_page_finish(docg3);
	return ret;
}

static int doc_write_oob(struct mtd_info *mtd, loff_t ofs,
			 struct mtd_oob_ops *ops)
{
	struct docg3 *docg3 = mtd->priv;
	int block0, block1, page, ret, pofs = 0, autoecc, oobdelta;
	u8 *oobbuf = ops->oobbuf;
	u8 *buf = ops->datbuf;
	size_t len, ooblen;
	u8 oob[DOC_LAYOUT_OOB_SIZE] __aligned(4);

	if (buf)
		len = ops->len;
	else
		len = 0;
	if (oobbuf)
		ooblen = ops->ooblen;
	else
		ooblen = 0;

	if (oobbuf && ops->mode == MTD_OPS_PLACE_OOB)
		oobbuf += ops->ooboffs;

	doc_dbg("doc_write_oob(from=%lld, mode=%d, data=(%p:%zu), oob=(%p:%zu))\n",
		ofs, ops->mode, buf, len, oobbuf, ooblen);
	switch (ops->mode) {
	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_RAW:
		oobdelta = mtd->oobsize;
		break;
	case MTD_OPS_AUTO_OOB:
		oobdelta = mtd->ecclayout->oobavail;
		break;
	default:
		oobdelta = 0;
	}
	if ((len % DOC_LAYOUT_PAGE_SIZE) || (ooblen % oobdelta) ||
	    (ofs % DOC_LAYOUT_PAGE_SIZE))
		return -EINVAL;
	if (len && ooblen &&
	    (len / DOC_LAYOUT_PAGE_SIZE) != (ooblen / oobdelta))
		return -EINVAL;

	ret = -EINVAL;
	calc_block_sector(ofs + len, &block0, &block1, &page, &pofs,
			  docg3->reliable);
	if (block1 > docg3->max_block)
		goto err;

	ops->oobretlen = 0;
	ops->retlen = 0;
	ret = 0;
	if (len == 0 && ooblen == 0)
		return -EINVAL;
	if (len == 0 && ooblen > 0)
		return doc_backup_oob(docg3, ofs, ops);

	autoecc = doc_guess_autoecc(ops);
	if (autoecc < 0)
		return autoecc;

	while (!ret && len > 0) {
		memset(oob, 0, sizeof(oob));
		if (ofs == docg3->oob_write_ofs)
			memcpy(oob, docg3->oob_write_buf, DOC_LAYOUT_OOB_SIZE);
		else if (ooblen > 0 && ops->mode == MTD_OPS_AUTO_OOB)
			doc_fill_autooob(oob, oobbuf);
		else if (ooblen > 0)
			memcpy(oob, oobbuf, DOC_LAYOUT_OOB_SIZE);
		ret = doc_write_page(docg3, ofs, buf, oob, autoecc);

		ofs += DOC_LAYOUT_PAGE_SIZE;
		len -= DOC_LAYOUT_PAGE_SIZE;
		buf += DOC_LAYOUT_PAGE_SIZE;
		if (ooblen) {
			oobbuf += oobdelta;
			ooblen -= oobdelta;
			ops->oobretlen += oobdelta;
		}
		ops->retlen += DOC_LAYOUT_PAGE_SIZE;
	}
err:
	doc_set_device_id(docg3, 0);
	return ret;
}

static int doc_write(struct mtd_info *mtd, loff_t to, size_t len,
		     size_t *retlen, const u_char *buf)
{
	struct docg3 *docg3 = mtd->priv;
	int ret;
	struct mtd_oob_ops ops;

	doc_dbg("doc_write(to=%lld, len=%zu)\n", to, len);
	ops.datbuf = (char *)buf;
	ops.len = len;
	ops.mode = MTD_OPS_PLACE_OOB;
	ops.oobbuf = NULL;
	ops.ooblen = 0;
	ops.ooboffs = 0;

	ret = doc_write_oob(mtd, to, &ops);
	*retlen = ops.retlen;
	return ret;
}
#endif

static void __init doc_set_driver_info(int chip_id, struct mtd_info *mtd)
{
	struct docg3 *docg3 = mtd->priv;
	int cfg;

	cfg = doc_register_readb(docg3, DOC_CONFIGURATION);
	docg3->if_cfg = (cfg & DOC_CONF_IF_CFG ? 1 : 0);
	docg3->reliable = reliable_mode;

	switch (chip_id) {
	case DOC_CHIPID_G3:
		mtd->name = asprintf("DiskOnChip G3 floor %d",
				      docg3->device_id);
		docg3->max_block = 2047;
		break;
	}
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->size = (docg3->max_block + 1) * DOC_LAYOUT_BLOCK_SIZE;
	if (docg3->reliable == 2)
		mtd->size /= 2;
	mtd->erasesize = DOC_LAYOUT_BLOCK_SIZE * DOC_LAYOUT_NBPLANES;
	if (docg3->reliable == 2)
		mtd->erasesize /= 2;
	mtd->writesize = DOC_LAYOUT_PAGE_SIZE;
	mtd->oobsize = DOC_LAYOUT_OOB_SIZE;
	mtd->read = doc_read;
	mtd->read_oob = doc_read_oob;
	mtd->block_isbad = doc_block_isbad;
#ifdef CONFIG_MTD_WRITE
	mtd->erase = doc_erase;
	mtd->write = doc_write;
	mtd->write_oob = doc_write_oob;
#endif
}

static struct mtd_info *doc_probe_device(void __iomem *base, int floor,
					 struct device_d *dev)
{
	int ret, bbt_nbpages;
	u16 chip_id, chip_id_inv;
	struct docg3 *docg3;
	struct mtd_info *mtd;

	ret = -ENOMEM;
	docg3 = kzalloc(sizeof(struct docg3), GFP_KERNEL);
	if (!docg3)
		goto nomem1;
	mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd)
		goto nomem2;
	mtd->priv = docg3;
	bbt_nbpages = DIV_ROUND_UP(docg3->max_block + 1,
				   8 * DOC_LAYOUT_PAGE_SIZE);
	docg3->bbt = kzalloc(bbt_nbpages * DOC_LAYOUT_PAGE_SIZE, GFP_KERNEL);
	if (!docg3->bbt)
		goto nomem3;

	docg3->dev = dev;
	docg3->device_id = floor;
	docg3->base = base;
	doc_set_device_id(docg3, docg3->device_id);
	if (!floor)
		doc_set_asic_mode(docg3, DOC_ASICMODE_RESET);
	doc_set_asic_mode(docg3, DOC_ASICMODE_NORMAL);

	chip_id = doc_register_readw(docg3, DOC_CHIPID);
	chip_id_inv = doc_register_readw(docg3, DOC_CHIPID_INV);

	ret = 0;
	if (chip_id != (u16)(~chip_id_inv))
		goto nomem3;

	switch (chip_id) {
	case DOC_CHIPID_G3:
		doc_info("Found a G3 DiskOnChip at addr %p, floor %d\n",
			 base, floor);
		break;
	default:
		doc_err("Chip id %04x is not a DiskOnChip G3 chip\n", chip_id);
		goto nomem3;
	}

	doc_set_driver_info(chip_id, mtd);

	doc_hamming_ecc_init(docg3, DOC_LAYOUT_OOB_PAGEINFO_SZ);
	doc_reload_bbt(docg3);
	return mtd;

nomem3:
	kfree(mtd);
nomem2:
	kfree(docg3);
nomem1:
	return ERR_PTR(ret);
}

static int __init docg3_probe(struct device_d *dev)
{
	struct mtd_info *mtd;
	void __iomem *base;
	int ret, floor, found = 0;

	base = dev_request_mem_region(dev, 0);

	ret = -ENOMEM;
	docg3_bch = init_bch(DOC_ECC_BCH_M, DOC_ECC_BCH_T,
			     DOC_ECC_BCH_PRIMPOLY);
	if (!docg3_bch)
		goto nomem2;

	/* for (floor = 0; floor < DOC_MAX_NBFLOORS; floor++) { */
	for (floor = 0; floor < 2; floor++) {
		mtd = doc_probe_device(base, floor, dev);
		if (IS_ERR(mtd)) {
			ret = PTR_ERR(mtd);
			goto err_probe;
		}
		if (!mtd) {
			if (floor == 0)
				goto notfound;
			else
				continue;
		}
		docg3_floors[floor] = mtd;
		mtd->parent = dev;
		ret = add_mtd_device(mtd, NULL);
		if (ret)
			goto err_probe;
		found++;
	}

	if (ret)
		goto err_probe;
	if (!found)
		goto notfound;

	return 0;

notfound:
	ret = -ENODEV;
	dev_info(dev, "No supported DiskOnChip found\n");
err_probe:
	free_bch(docg3_bch);
nomem2:
	return ret;
}

static struct driver_d  g3_driver = {
	.name	= "docg3",
	.probe	= docg3_probe,
};
device_platform_driver(g3_driver);
