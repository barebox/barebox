/*
 *  drivers/mtd/nand.c
 *
 *  Overview:
 *   This is the generic MTD driver for NAND flash devices. It should be
 *   capable of working with almost all NAND chips currently available.
 *   Basic support for AG-AND chips is provided.
 *
 *	Additional technical information is available on
 *	http://www.linux-mtd.infradead.org/doc/nand.html
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *		  2002-2006 Thomas Gleixner (tglx@linutronix.de)
 *
 *  Credits:
 *	David Woodhouse for adding multichip support
 *
 *	Aleph One Ltd. and Toby Churchill Ltd. for supporting the
 *	rework for 2K page size chips
 *
 *  TODO:
 *	Enable cached programming for 2k page size chips
 *	Check, if mtd->ecctype should be set to MTD_ECC_HW
 *	if we have HW ecc support.
 *	The AG-AND chips have nice features for speed improvement,
 *	which are not supported yet. Read / program 4 pages in one go.
 *	BBT table is not serialized, has to be fixed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt) "nand: " fmt

#include <common.h>
#include <errno.h>
#include <clock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/err.h>
#include <linux/mtd/nand_ecc.h>
#include <asm/byteorder.h>
#include <io.h>
#include <malloc.h>
#include <module.h>

#include "nand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* Define default oob placement schemes for large and small page devices */
static struct nand_ecclayout nand_oob_8 = {
	.eccbytes = 3,
	.eccpos = {0, 1, 2},
	.oobfree = {
		{.offset = 3,
		 .length = 2},
		{.offset = 6,
		 .length = 2}}
};

static struct nand_ecclayout nand_oob_16 = {
	.eccbytes = 6,
	.eccpos = {0, 1, 2, 3, 6, 7},
	.oobfree = {
		{.offset = 8,
		 . length = 8}}
};

static struct nand_ecclayout nand_oob_64 = {
	.eccbytes = 24,
	.eccpos = {
		   40, 41, 42, 43, 44, 45, 46, 47,
		   48, 49, 50, 51, 52, 53, 54, 55,
		   56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 38}}
};

#define DEFINE_LED_TRIGGER(x)
#define DEFINE_LED_TRIGGER_GLOBAL(x)
#define led_trigger_register_simple(x, y) do {} while(0)
#define led_trigger_unregister_simple(x) do {} while(0)
#define led_trigger_event(x, y) do {} while(0)

/*
 * For devices which display every fart in the system on a separate LED. Is
 * compiled away when LED support is disabled.
 */
DEFINE_LED_TRIGGER(nand_led_trigger);

/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 8bit buswith
 */
static uint8_t nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return readb(chip->IO_ADDR_R);
}

/**
 * nand_read_byte16 - [DEFAULT] read one byte endianess aware from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith with
 * endianess conversion
 */
static uint8_t nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return (uint8_t) cpu_to_le16(readw(chip->IO_ADDR_R));
}

/**
 * nand_read_word - [DEFAULT] read one word from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith without
 * endianess conversion
 */
static u16 nand_read_word(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return readw(chip->IO_ADDR_R);
}

/**
 * nand_select_chip - [DEFAULT] control CE line
 * @mtd:	MTD device structure
 * @chipnr:	chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd->priv;

	switch (chipnr) {
	case -1:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
		break;
	case 0:
		break;
	default:
		printf("%s: illegal chip number %d\n", __func__, chipnr);
	}
}

/**
 * nand_read_buf - [DEFAULT] read chip data into buffer
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 8bit buswith
 */
static void nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	for (i = 0; i < len; i++)
		buf[i] = readb(chip->IO_ADDR_R);
}

/**
 * nand_verify_buf - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 8bit buswith
 */
static int nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	for (i = 0; i < len; i++)
		if (buf[i] != readb(chip->IO_ADDR_R))
			return -EFAULT;
	return 0;
}

/**
 * nand_read_buf16 - [DEFAULT] read chip data into buffer
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 16bit buswith
 */
static void nand_read_buf16(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		p[i] = readw(chip->IO_ADDR_R);
}

/**
 * nand_verify_buf16 - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 16bit buswith
 */
static int nand_verify_buf16(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		if (p[i] != readw(chip->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

/**
 * nand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 *
 * Check, if the block is bad.
 */
static int nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int page, chipnr, res = 0;
	struct nand_chip *chip = mtd->priv;
	u16 bad;

	page = (int)(ofs >> chip->page_shift) & chip->pagemask;

	if (getchip) {
		chipnr = (int)(ofs >> chip->chip_shift);

		/* Select the NAND device */
		chip->select_chip(mtd, chipnr);
	}

	if (chip->options & NAND_BUSWIDTH_16) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos & 0xFE,
			      page);
		bad = cpu_to_le16(chip->read_word(mtd));
		if (chip->badblockpos & 0x1)
			bad >>= 8;
		if ((bad & 0xFF) != 0xff)
			res = 1;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos, page);
		if (chip->read_byte(mtd) != 0xff)
			res = 1;
	}

	return res;
}

/**
 * nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 * @allowbbt:	1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
int nand_block_checkbad(struct mtd_info *mtd, loff_t ofs, int getchip,
			       int allowbbt)
{
	struct nand_chip *chip = mtd->priv;

#ifdef CONFIG_NAND_BBT
	if (!chip->bbt)
		return chip->block_bad(mtd, ofs, getchip);
	/* Return info from the table */
	return nand_isbad_bbt(mtd, ofs, allowbbt);
#else
	return chip->block_bad(mtd, ofs, getchip);
#endif
}

/*
 * Wait for the ready pin, after a command
 * The timeout is catched later.
 */
void nand_wait_ready(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	uint64_t start = get_time_ns();

	led_trigger_event(nand_led_trigger, LED_FULL);
	/* wait until command is processed or timeout occures */
	do {
		if (chip->dev_ready(mtd))
			break;
	} while (!is_timeout(start, SECOND * 2));
	led_trigger_event(nand_led_trigger, LED_OFF);
}
EXPORT_SYMBOL(nand_wait_ready);

/**
 * nand_command - [DEFAULT] Send command to NAND device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small page
 * devices (256/512 Bytes per page)
 */
static void nand_command(struct mtd_info *mtd, unsigned int command,
			 int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	int ctrl = NAND_CTRL_CLE | NAND_CTRL_CHANGE;

	/*
	 * Write out the command to the device.
	 */
	if (command == NAND_CMD_SEQIN) {
		int readcmd;

		if (column >= mtd->writesize) {
			/* OOB area */
			column -= mtd->writesize;
			readcmd = NAND_CMD_READOOB;
		} else if (column < 256) {
			/* First 256 bytes --> READ0 */
			readcmd = NAND_CMD_READ0;
		} else {
			column -= 256;
			readcmd = NAND_CMD_READ1;
		}
		chip->cmd_ctrl(mtd, readcmd, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
	}
	chip->cmd_ctrl(mtd, command, ctrl);

	/*
	 * Address cycle, when necessary
	 */
	ctrl = NAND_CTRL_ALE | NAND_CTRL_CHANGE;
	/* Serially input address */
	if (column != -1) {
		/* Adjust columns for 16 bit buswidth */
		if (chip->options & NAND_BUSWIDTH_16)
			column >>= 1;
		chip->cmd_ctrl(mtd, column, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
	}
	if (page_addr != -1) {
		chip->cmd_ctrl(mtd, page_addr, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
		chip->cmd_ctrl(mtd, page_addr >> 8, ctrl);
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20))
			chip->cmd_ctrl(mtd, page_addr >> 16, ctrl);
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	 */
	switch (command) {

	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
		return;

	case NAND_CMD_RESET:
		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			       NAND_CTRL_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd,
			       NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
		return;

		/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return;
		}
	}
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	nand_wait_ready(mtd);
}

/**
 * nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page
 * devices We dont have the separate regions as we have in the small page
 * devices.  We must emulate NAND_CMD_READOOB to keep the code compatible.
 */
static void nand_command_lp(struct mtd_info *mtd, unsigned int command,
			    int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}

	/* Command latch cycle */
	chip->cmd_ctrl(mtd, command & 0xff,
		       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (chip->options & NAND_BUSWIDTH_16)
				column >>= 1;
			chip->cmd_ctrl(mtd, column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;
			chip->cmd_ctrl(mtd, column >> 8, ctrl);
		}
		if (page_addr != -1) {
			chip->cmd_ctrl(mtd, page_addr, ctrl);
			chip->cmd_ctrl(mtd, page_addr >> 8,
				       NAND_NCE | NAND_ALE);
			/* One more address cycle for devices > 128MiB */
			if (chip->chipsize > (128 << 20))
				chip->cmd_ctrl(mtd, page_addr >> 16,
					       NAND_NCE | NAND_ALE);
		}
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * program and erase have their own busy handlers
	 * status, sequential in, and deplete1 need no delay
	 */
	switch (command) {

	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_RNDIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_DEPLETE1:
		return;

		/*
		 * read error status commands require only a short delay
		 */
	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
		udelay(chip->chip_delay);
		return;

	case NAND_CMD_RESET:
		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
		return;

	case NAND_CMD_RNDOUT:
		/* No ready / busy check necessary */
		chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		return;

	case NAND_CMD_READ0:
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);

		/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return;
		}
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	nand_wait_ready(mtd);
}

/**
 * nand_wait - [DEFAULT]  wait until the command is done
 * @mtd:	MTD device structure
 * @chip:	NAND chip structure
 *
 * Wait for command done. This applies to erase and program only
 * Erase can take up to 400ms and program up to 20ms according to
 * general NAND and SmartMedia specs
 */
static int nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{

	uint64_t start = get_time_ns();
	uint64_t timeo;
	int status, state = chip->state;

	if (state == FL_ERASING)
		timeo = 400 * MSECOND;
	else
		timeo = 20 * MSECOND;

	led_trigger_event(nand_led_trigger, LED_FULL);

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	if ((state == FL_ERASING) && (chip->options & NAND_IS_AND))
		chip->cmdfunc(mtd, NAND_CMD_STATUS_MULTI, -1, -1);
	else
		chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	while (!is_timeout(start, timeo)) {
		if (chip->dev_ready) {
			if (chip->dev_ready(mtd))
				break;
		} else {
			if (chip->read_byte(mtd) & NAND_STATUS_READY)
				break;
		}
	}
	led_trigger_event(nand_led_trigger, LED_OFF);

	status = (int)chip->read_byte(mtd);
	return status;
}

/**
 * nand_read_page_raw - [Intern] read raw page data without ecc
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 */
static int nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
			      uint8_t *buf)
{
	chip->read_buf(mtd, buf, mtd->writesize);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	return 0;
}

/**
 * nand_transfer_oob - [Internal] Transfer oob to client buffer
 * @chip:	nand chip structure
 * @oob:	oob destination address
 * @ops:	oob ops structure
 * @len:	size of oob to transfer
 */
#ifdef CONFIG_NAND_READ_OOB
static uint8_t *nand_transfer_oob(struct nand_chip *chip, uint8_t *oob,
				  struct mtd_oob_ops *ops, size_t len)
{
	switch(ops->mode) {

	case MTD_OOB_PLACE:
	case MTD_OOB_RAW:
		memcpy(oob, chip->oob_poi + ops->ooboffs, len);
		return oob + len;

	case MTD_OOB_AUTO: {
		struct nand_oobfree *free = chip->ecc.layout->oobfree;
		uint32_t boffs = 0, roffs = ops->ooboffs;
		size_t bytes = 0;

		for(; free->length && len; free++, len -= bytes) {
			/* Read request not from offset 0 ? */
			if (unlikely(roffs)) {
				if (roffs >= free->length) {
					roffs -= free->length;
					continue;
				}
				boffs = free->offset + roffs;
				bytes = min_t(size_t, len,
					      (free->length - roffs));
				roffs = 0;
			} else {
				bytes = min_t(size_t, len, free->length);
				boffs = free->offset;
			}
			memcpy(oob, chip->oob_poi + boffs, bytes);
			oob += bytes;
		}
		return oob;
	}
	default:
		BUG();
	}
	return NULL;
}
#endif

/**
 * nand_do_read_ops - [Internal] Read data with ECC
 *
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @ops:	oob ops structure
 *
 * Internal function. Called with chip held.
 */
static int nand_do_read_ops(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	int chipnr, page, realpage, col, bytes, aligned;
	struct nand_chip *chip = mtd->priv;
	struct mtd_ecc_stats stats;
	int blkcheck = (1 << (chip->phys_erase_shift - chip->page_shift)) - 1;
	int sndcmd = 1;
	int ret = 0;
	uint32_t readlen = ops->len;
	uint32_t oobreadlen = ops->ooblen;
	uint8_t *bufpoi, *oob, *buf;

	stats = mtd->ecc_stats;

	chipnr = (int)(from >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);

	realpage = (int)(from >> chip->page_shift);
	page = realpage & chip->pagemask;

	col = (int)(from & (mtd->writesize - 1));

	buf = ops->datbuf;
	oob = ops->oobbuf;

	while(1) {
		bytes = min(mtd->writesize - col, readlen);
		aligned = (bytes == mtd->writesize);

		/* Is the current page in the buffer ? */
		if (realpage != chip->pagebuf || oob) {
			bufpoi = aligned ? buf : chip->buffers->databuf;

			if (likely(sndcmd)) {
				chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
				sndcmd = 0;
			}

			/* Now read the page into the buffer */
			if (unlikely(ops->mode == MTD_OOB_RAW))
				ret = chip->ecc.read_page_raw(mtd, chip, bufpoi);
			else
				ret = chip->ecc.read_page(mtd, chip, bufpoi);
			if (ret < 0)
				break;

			/* Transfer not aligned data */
			if (!aligned) {
				chip->pagebuf = realpage;
				memcpy(buf, chip->buffers->databuf + col, bytes);
			}

			buf += bytes;

#ifdef CONFIG_NAND_READ_OOB
			if (unlikely(oob)) {
				/* Raw mode does data:oob:data:oob */
				if (ops->mode != MTD_OOB_RAW) {
					int toread = min(oobreadlen,
						chip->ecc.layout->oobavail);
					if (toread) {
						oob = nand_transfer_oob(chip,
							oob, ops, toread);
						oobreadlen -= toread;
					}
				} else
					buf = nand_transfer_oob(chip,
						buf, ops, mtd->oobsize);
			}
#endif
			if (!(chip->options & NAND_NO_READRDY)) {
				/*
				 * Apply delay or wait for ready/busy pin. Do
				 * this before the AUTOINCR check, so no
				 * problems arise if a chip which does auto
				 * increment is marked as NOAUTOINCR by the
				 * board driver.
				 */
				if (!chip->dev_ready)
					udelay(chip->chip_delay);
				else
					nand_wait_ready(mtd);
			}
		} else {
			memcpy(buf, chip->buffers->databuf + col, bytes);
			buf += bytes;
		}

		readlen -= bytes;

		if (!readlen)
			break;

		/* For subsequent reads align to page boundary. */
		col = 0;
		/* Increment page address */
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}

		/* Check, if the chip supports auto page increment
		 * or if we have hit a block boundary.
		 */
		if (!NAND_CANAUTOINCR(chip) || !(page & blkcheck))
			sndcmd = 1;
	}

	ops->retlen = ops->len - (size_t) readlen;
	if (oob)
		ops->oobretlen = ops->ooblen - oobreadlen;

	if (ret)
		return ret;

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;

	return 0;
}

/**
 * nand_read - [MTD Interface] MTD compability function for nand_do_read_ecc
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 *
 * Get hold of the chip and call nand_do_read
 */
static int nand_read(struct mtd_info *mtd, loff_t from, size_t len,
		     size_t *retlen, uint8_t *buf)
{
	struct nand_chip *chip = mtd->priv;
	int ret;

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size)
		return -EINVAL;
	if (!len)
		return 0;

	chip->ops.len = len;
	chip->ops.datbuf = buf;
	chip->ops.oobbuf = NULL;

	ret = nand_do_read_ops(mtd, from, &chip->ops);

	*retlen = chip->ops.retlen;

	return ret;
}

/**
 * nand_read_oob_std - [REPLACABLE] the most common OOB data read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to read
 * @sndcmd:	flag whether to issue read command or not
 */
int nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd)
{
	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		sndcmd = 0;
	}
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	return sndcmd;
}

/**
 * nand_do_read_oob - [Intern] NAND read out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @ops:	oob operations description structure
 *
 * NAND read out-of-band data from the spare area
 */
#ifdef CONFIG_NAND_READ_OOB
static int nand_do_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	int page, realpage, chipnr, sndcmd = 1;
	struct nand_chip *chip = mtd->priv;
	int blkcheck = (1 << (chip->phys_erase_shift - chip->page_shift)) - 1;
	int readlen = ops->ooblen;
	int len;
	uint8_t *buf = ops->oobbuf;

	MTD_DEBUG(MTD_DEBUG_LEVEL3, "nand_read_oob: from = 0x%08Lx, len = %i\n",
	      (unsigned long long)from, readlen);

	if (ops->mode == MTD_OOB_AUTO)
		len = chip->ecc.layout->oobavail;
	else
		len = mtd->oobsize;

	if (unlikely(ops->ooboffs >= len)) {
		MTD_DEBUG(MTD_DEBUG_LEVEL0, "nand_read_oob: "
			"Attempt to start read outside oob\n");
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size ||
		     ops->ooboffs + readlen > ((mtd->size >> chip->page_shift) -
					(from >> chip->page_shift)) * len)) {
		MTD_DEBUG(MTD_DEBUG_LEVEL0, "nand_read_oob: "
			"Attempt read beyond end of device\n");
		return -EINVAL;
	}

	chipnr = (int)(from >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);

	/* Shift to get page */
	realpage = (int)(from >> chip->page_shift);
	page = realpage & chip->pagemask;

	while(1) {
		sndcmd = chip->ecc.read_oob(mtd, chip, page, sndcmd);

		len = min(len, readlen);
		buf = nand_transfer_oob(chip, buf, ops, len);

		if (!(chip->options & NAND_NO_READRDY)) {
			/*
			 * Apply delay or wait for ready/busy pin. Do this
			 * before the AUTOINCR check, so no problems arise if a
			 * chip which does auto increment is marked as
			 * NOAUTOINCR by the board driver.
			 */
			if (!chip->dev_ready)
				udelay(chip->chip_delay);
			else
				nand_wait_ready(mtd);
		}

		readlen -= len;
		if (!readlen)
			break;

		/* Increment page address */
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}

		/* Check, if the chip supports auto page increment
		 * or if we have hit a block boundary.
		 */
		if (!NAND_CANAUTOINCR(chip) || !(page & blkcheck))
			sndcmd = 1;
	}

	ops->oobretlen = ops->ooblen;
	return 0;
}

/**
 * nand_read_oob - [MTD Interface] NAND read data and/or out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @ops:	oob operation description structure
 *
 * NAND read data and/or out-of-band data
 */
static int nand_read_oob(struct mtd_info *mtd, loff_t from,
			 struct mtd_oob_ops *ops)
{
	int ret = -ENOSYS;

	ops->retlen = 0;

	/* Do not allow reads past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->size) {
		MTD_DEBUG(MTD_DEBUG_LEVEL0, "nand_read_oob: "
		      "Attempt read beyond end of device\n");
		return -EINVAL;
	}

	switch(ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
	case MTD_OOB_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf)
		ret = nand_do_read_oob(mtd, from, ops);
	else
		ret = nand_do_read_ops(mtd, from, ops);

 out:
	return ret;
}
#endif

/**
 * nand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd:	MTD device structure
 * @offs:	offset relative to mtd start
 */
int nand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	/* Check for invalid offset */
	if (offs > mtd->size)
		return -EINVAL;

	return nand_block_checkbad(mtd, offs, 1, 0);
}

/*
 * Set default functions
 */
static void nand_set_defaults(struct nand_chip *chip, int busw)
{
	/* check for proper chip_delay setup, set 20us if not */
	if (!chip->chip_delay)
		chip->chip_delay = 20;

	/* check, if a user supplied command function given */
	if (chip->cmdfunc == NULL)
		chip->cmdfunc = nand_command;

	/* check, if a user supplied wait function given */
	if (chip->waitfunc == NULL)
		chip->waitfunc = nand_wait;

	if (!chip->select_chip)
		chip->select_chip = nand_select_chip;
	if (!chip->read_byte || chip->read_byte == nand_read_byte)
		chip->read_byte = busw ? nand_read_byte16 : nand_read_byte;
	if (!chip->read_word)
		chip->read_word = nand_read_word;
	if (!chip->block_bad)
		chip->block_bad = nand_block_bad;
#ifdef CONFIG_MTD_WRITE
	if (!chip->block_markbad)
		chip->block_markbad = nand_default_block_markbad;
	if (!chip->write_buf || chip->write_buf == nand_write_buf)
		chip->write_buf = busw ? nand_write_buf16 : nand_write_buf;
#endif
	if (!chip->read_buf || chip->read_buf == nand_read_buf)
		chip->read_buf = busw ? nand_read_buf16 : nand_read_buf;
	if (!chip->verify_buf || chip->verify_buf == nand_verify_buf)
		chip->verify_buf = busw ? nand_verify_buf16 : nand_verify_buf;
#ifdef CONFIG_NAND_BBT
	if (!chip->scan_bbt)
		chip->scan_bbt = nand_default_bbt;
#endif
	if (!chip->controller) {
		chip->controller = &chip->hwcontrol;
	}

}

/*
 * sanitize ONFI strings so we can safely print them
 */
static void sanitize_string(char *s, size_t len)
{
	ssize_t i;

	/* null terminate */
	s[len - 1] = 0;

	/* remove non printable chars */
	for (i = 0; i < len - 1; i++) {
		if (s[i] < ' ' || s[i] > 127)
			s[i] = '?';
	}

	/* remove trailing spaces */
	strim(s);
}

static u16 onfi_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}

/*
 * Check if the NAND chip is ONFI compliant, returns 1 if it is, 0 otherwise
 */
static int nand_flash_detect_onfi(struct mtd_info *mtd, struct nand_chip *chip,
					int *busw)
{
	struct nand_onfi_params *p = &chip->onfi_params;
	int i;
	int val;

	/* try ONFI for unknow chip or LP */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x20, -1);
	if (chip->read_byte(mtd) != 'O' || chip->read_byte(mtd) != 'N' ||
		chip->read_byte(mtd) != 'F' || chip->read_byte(mtd) != 'I')
		return 0;

	pr_info("ONFI flash detected ... ");
	chip->cmdfunc(mtd, NAND_CMD_PARAM, 0, -1);
	for (i = 0; i < 3; i++) {
		chip->read_buf(mtd, (uint8_t *)p, sizeof(*p));
		if (onfi_crc16(ONFI_CRC_BASE, (uint8_t *)p, 254) ==
				le16_to_cpu(p->crc)) {
			pr_info("ONFI param page %d valid\n", i);
			break;
		}
	}

	if (i == 3) {
		pr_info("no valid ONFI param page found\n");
		return 0;
	}

	/* check version */
	val = le16_to_cpu(p->revision);
	if (val & (1 << 5))
		chip->onfi_version = 23;
	else if (val & (1 << 4))
		chip->onfi_version = 22;
	else if (val & (1 << 3))
		chip->onfi_version = 21;
	else if (val & (1 << 2))
		chip->onfi_version = 20;
	else if (val & (1 << 1))
		chip->onfi_version = 10;
	else
		chip->onfi_version = 0;

	if (!chip->onfi_version) {
		pr_info("unsupported ONFI version: %d\n", val);
		return 0;
	}

	sanitize_string(p->manufacturer, sizeof(p->manufacturer));
	sanitize_string(p->model, sizeof(p->model));
	if (!mtd->name)
		mtd->name = p->model;
	mtd->writesize = le32_to_cpu(p->byte_per_page);
	mtd->erasesize = le32_to_cpu(p->pages_per_block) * mtd->writesize;
	mtd->oobsize = le16_to_cpu(p->spare_bytes_per_page);
	chip->chipsize = (uint64_t)le32_to_cpu(p->blocks_per_lun) * mtd->erasesize;
	*busw = 0;
	if (le16_to_cpu(p->features) & 1)
		*busw = NAND_BUSWIDTH_16;

	chip->options &= ~NAND_CHIPOPTIONS_MSK;
	chip->options |= NAND_NO_READRDY & NAND_CHIPOPTIONS_MSK;

	return 1;
}

/*
 * Get the flash and manufacturer id and lookup if the type is supported
 */
static struct nand_flash_dev *nand_get_flash_type(struct mtd_info *mtd,
						  struct nand_chip *chip,
						  int busw, int *maf_id)
{
	struct nand_flash_dev *type = NULL;
	int i, dev_id, maf_idx;
	int id_data[8];
	int ret;

	/* Select the device */
	chip->select_chip(mtd, 0);

	/*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up
	 */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Send the command for reading device ID */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	*maf_id = chip->read_byte(mtd);
	dev_id = chip->read_byte(mtd);

	/* Try again to make sure, as some systems the bus-hold or other
	 * interface concerns can cause random data which looks like a
	 * possibly credible NAND flash to appear. If the two results do
	 * not match, ignore the device completely.
	 */

	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */

	id_data[0] = chip->read_byte(mtd);
	id_data[1] = chip->read_byte(mtd);

	if (id_data[0] != *maf_id || id_data[1] != dev_id) {
		pr_err("%s: second ID read did not match "
		       "%02x,%02x against %02x,%02x\n", __func__,
		       *maf_id, dev_id, id_data[0], id_data[1]);
		return ERR_PTR(-ENODEV);
	}

	/* Lookup the flash id */
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {
		if (dev_id == nand_flash_ids[i].id) {
			type =  &nand_flash_ids[i];
			break;
		}
	}

	chip->onfi_version = 0;
	if (!type) {
		/* Check is chip is ONFI compliant */
		ret = nand_flash_detect_onfi(mtd, chip, &busw);
		if (ret)
			goto ident_done;
		else {
			pr_err("NAND type unknown: %02x,%02x\n", *maf_id, dev_id);
			return ERR_PTR(-ENODEV);
		}
	}

	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read entire ID string */

	for (i = 0; i < 8; i++)
		id_data[i] = chip->read_byte(mtd);

	if (!mtd->name)
		mtd->name = type->name;

	chip->chipsize = type->chipsize << 20;

	if (!type->pagesize) {
		int extid;
		/* The 3rd id byte holds MLC / multichip data */
		chip->cellinfo = id_data[2];
		/* The 4th id byte is the important one */
		extid = id_data[3];

		/*
		 * Field definitions are in the following datasheets:
		 * Old style (4,5 byte ID): Samsung K9GAG08U0M (p.32)
		 * New style   (6 byte ID): Samsung K9GBG08U0M (p.40)
		 *
		 * Check for wraparound + Samsung ID + nonzero 6th byte
		 * to decide what to do.
		 */
		if (id_data[0] == id_data[6] && id_data[1] == id_data[7] &&
				id_data[0] == NAND_MFR_SAMSUNG &&
				(chip->cellinfo & NAND_CI_CELLTYPE_MSK) &&
				id_data[5] != 0x00) {
			/* Calc pagesize */
			mtd->writesize = 2048 << (extid & 0x03);
			extid >>= 2;
			/* Calc oobsize */
			switch (extid & 0x03) {
			case 1:
				mtd->oobsize = 128;
				break;
			case 2:
				mtd->oobsize = 218;
				break;
			case 3:
				mtd->oobsize = 400;
				break;
			default:
				mtd->oobsize = 436;
				break;
			}
			extid >>= 2;
			/* Calc blocksize */
			mtd->erasesize = (128 * 1024) <<
				(((extid >> 1) & 0x04) | (extid & 0x03));
			busw = 0;
		} else {
			/* Calc pagesize */
			mtd->writesize = 1024 << (extid & 0x03);
			extid >>= 2;
			/* Calc oobsize */
			mtd->oobsize = (8 << (extid & 0x01)) *
				(mtd->writesize >> 9);
			extid >>= 2;
			/* Calc blocksize. Blocksize is multiples of 64KiB */
			mtd->erasesize = (64 * 1024) << (extid & 0x03);
			extid >>= 2;
			/* Get buswidth information */
			busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
		}


	} else {
		/*
		 * Old devices have chip data hardcoded in the device id table
		 */
		mtd->erasesize = type->erasesize;
		mtd->writesize = type->pagesize;
		mtd->oobsize = mtd->writesize / 32;
		busw = type->options & NAND_BUSWIDTH_16;

		/*
		 * Check for Spansion/AMD ID + repeating 5th, 6th byte since
		 * some Spansion chips have erasesize that conflicts with size
		 * listed in nand_ids table
		 * Data sheet (5 byte ID): Spansion S30ML-P ORNAND (p.39)
		 */
		if (*maf_id == NAND_MFR_AMD && id_data[4] != 0x00 &&
				id_data[5] == 0x00 && id_data[6] == 0x00 &&
				id_data[7] == 0x00 && mtd->writesize == 512) {
			mtd->erasesize = 128 * 1024;
			mtd->erasesize <<= ((id_data[3] & 0x03) << 1);
		}
	}

	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	/* Get chip options, preserve non chip based options */
	chip->options &= ~NAND_CHIPOPTIONS_MSK;
	chip->options |= type->options & NAND_CHIPOPTIONS_MSK;

	/* Check if chip is a not a samsung device. Do not clear the
	 * options for chips which are not having an extended id.
	 */
	if (*maf_id != NAND_MFR_SAMSUNG && !type->pagesize)
		chip->options &= ~NAND_SAMSUNG_LP_OPTIONS;

ident_done:
	/*
	 * Set chip as a default. Board drivers can override it, if necessary
	 */
	chip->options |= NAND_NO_AUTOINCR;

	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	if (chip->options & NAND_BUSWIDTH_AUTO) {
		chip->options |= busw;
		nand_set_defaults(chip, busw);
		if (chip->set_buswidth)
			chip->set_buswidth(mtd, chip, busw);
	}

	/*
	 * Check, if buswidth is correct. Hardware drivers should set
	 * chip correct !
	 */
	if (busw != (chip->options & NAND_BUSWIDTH_16)) {
		pr_info("NAND device: Manufacturer ID:"
			" 0x%02x, Chip ID: 0x%02x (%s %s)\n", *maf_id,
			dev_id, nand_manuf_ids[maf_idx].name, mtd->name);
		pr_warning("NAND bus width %d instead %d bit\n",
			(chip->options & NAND_BUSWIDTH_16) ? 16 : 8,
			busw ? 16 : 8);
		return ERR_PTR(-EINVAL);
	}

	/* Calculate the address shift from the page size */
	chip->page_shift = ffs(mtd->writesize) - 1;
	/* Convert chipsize to number of pages per chip -1. */
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;

	chip->bbt_erase_shift = chip->phys_erase_shift =
		ffs(mtd->erasesize) - 1;
	if (chip->chipsize & 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else {
		chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32));
		chip->chip_shift += 32 - 1;
	}

	/* Set the bad block position */
	chip->badblockpos = mtd->writesize > 512 ?
		NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

#ifdef CONFIG_MTD_WRITE
	/* Check for AND chips with 4 page planes */
	if (chip->options & NAND_4PAGE_ARRAY)
		chip->erase_cmd = multi_erase_cmd;
	else
		chip->erase_cmd = single_erase_cmd;
#endif
	/* Do not replace user supplied command function ! */
	if (mtd->writesize > 512 && chip->cmdfunc == nand_command)
		chip->cmdfunc = nand_command_lp;

	pr_notice("Manufacturer ID: 0x%02x, Chip ID: 0x%02x (%s %s),"
		" page size: %d, OOB size: %d\n",
		*maf_id, dev_id, nand_manuf_ids[maf_idx].name,
		chip->onfi_version ? chip->onfi_params.model : type->name,
		mtd->writesize, mtd->oobsize);

	return type;
}

/**
 * nand_scan_ident - [NAND Interface] Scan for the NAND device
 * @mtd:	     MTD device structure
 * @maxchips:	     Number of chips to scan for
 *
 * This is the first phase of the normal nand_scan() function. It
 * reads the flash ID and sets up MTD fields accordingly.
 *
 * The mtd->owner field must be set to the module of the caller.
 */
int nand_scan_ident(struct mtd_info *mtd, int maxchips)
{
	int i, busw, nand_maf_id;
	struct nand_chip *chip = mtd->priv;
	struct nand_flash_dev *type;

	if (chip->options & NAND_BUSWIDTH_AUTO && !chip->set_buswidth) {
		pr_err("buswidth detection but no buswidth callback\n");
		return -EINVAL;
	}

	/* Get buswidth to select the correct functions */
	busw = chip->options & NAND_BUSWIDTH_16;
	/* Set the default functions */
	nand_set_defaults(chip, busw);

	/* Read the flash type */
	type = nand_get_flash_type(mtd, chip, busw, &nand_maf_id);

	if (IS_ERR(type)) {
		pr_warning("No NAND device found (%ld)!\n", PTR_ERR(type));
		chip->select_chip(mtd, -1);
		return PTR_ERR(type);
	}

	/* Check for a chip array */
	for (i = 1; i < maxchips; i++) {
		chip->select_chip(mtd, i);
		/* Send the command for reading device ID */
		chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
		/* Read manufacturer and device IDs */
		if (nand_maf_id != chip->read_byte(mtd) ||
		    type->id != chip->read_byte(mtd))
			break;
	}
	if (i > 1)
		pr_info("%d NAND chips detected\n", i);

	/* Store the number of chips and calc total size for mtd */
	chip->numchips = i;
	mtd->size = i * chip->chipsize;

	return 0;
}

static void __maybe_unused nand_check_hwecc(struct mtd_info *mtd, struct nand_chip *chip)
{
	if ((!chip->ecc.calculate || !chip->ecc.correct ||
	     !chip->ecc.hwctl) &&
	    (!chip->ecc.read_page || !chip->ecc.write_page)) {
		pr_warning("No ECC functions supplied, "
		       "Hardware ECC not possible\n");
		BUG();
	}

	if (mtd->writesize < chip->ecc.size) {
		pr_warning("%d byte HW ECC not possible on "
		       "%d byte page size\n",
			chip->ecc.size, mtd->writesize);
		BUG();
	}
}

/**
 * nand_scan_tail - [NAND Interface] Scan for the NAND device
 * @mtd:	    MTD device structure
 * @maxchips:	    Number of chips to scan for
 *
 * This is the second phase of the normal nand_scan() function. It
 * fills out all the uninitialized function pointers with the defaults
 * and scans for a bad block table if appropriate.
 */
int nand_scan_tail(struct mtd_info *mtd)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	if (!(chip->options & NAND_OWN_BUFFERS))
		chip->buffers = kmalloc(sizeof(*chip->buffers), GFP_KERNEL);
	if (!chip->buffers)
		return -ENOMEM;

	/* Set the internal oob buffer location, just after the page data */
	chip->oob_poi = chip->buffers->databuf + mtd->writesize;

	/*
	 * If no default placement scheme is given, select an appropriate one
	 */
	if (!chip->ecc.layout) {
		switch (mtd->oobsize) {
		case 8:
			chip->ecc.layout = &nand_oob_8;
			break;
		case 16:
			chip->ecc.layout = &nand_oob_16;
			break;
		case 64:
			chip->ecc.layout = &nand_oob_64;
			break;
		default:
			pr_warning("No oob scheme defined for "
			       "oobsize %d\n", mtd->oobsize);
			BUG();
		}
	}

#ifdef CONFIG_MTD_WRITE
	if (!chip->write_page)
		chip->write_page = nand_write_page;
#endif

	/*
	 * check ECC mode, default to software if 3byte/512byte hardware ECC is
	 * selected and we have 256 byte pagesize fallback to software ECC
	 */
	if (!chip->ecc.read_page_raw)
		chip->ecc.read_page_raw = nand_read_page_raw;
#ifdef CONFIG_MTD_WRITE
	if (!chip->ecc.write_page_raw)
		chip->ecc.write_page_raw = nand_write_page_raw;
#endif
	switch (chip->ecc.mode) {
#ifdef CONFIG_NAND_ECC_HW
	case NAND_ECC_HW:
		nand_check_hwecc(mtd, chip);
		nand_init_ecc_hw(chip);
		break;
#endif
#ifdef CONFIG_NAND_ECC_HW_SYNDROME
	case NAND_ECC_HW_SYNDROME:
		nand_check_hwecc(mtd, chip);
		nand_init_ecc_hw_syndrome(chip);
		break;
#endif
#ifdef CONFIG_NAND_ECC_SOFT
	case NAND_ECC_SOFT:
		nand_init_ecc_soft(chip);
		break;
#endif
#ifdef CONFIG_NAND_ECC_NONE
	case NAND_ECC_NONE:
		pr_warning("NAND_ECC_NONE selected by board driver. "
		       "This is not recommended !!\n");
		chip->ecc.read_page = nand_read_page_raw;
#ifdef CONFIG_MTD_WRITE
		chip->ecc.write_page = nand_write_page_raw;
		chip->ecc.write_oob = nand_write_oob_std;
#endif
		chip->ecc.read_oob = nand_read_oob_std;
		chip->ecc.size = mtd->writesize;
		chip->ecc.bytes = 0;
		break;
#endif
	default:
		pr_warning("Invalid NAND_ECC_MODE %d\n",
		       chip->ecc.mode);
		BUG();
	}

	/*
	 * The number of bytes available for a client to place data into
	 * the out of band area
	 */
	chip->ecc.layout->oobavail = 0;
	for (i = 0; chip->ecc.layout->oobfree[i].length; i++)
		chip->ecc.layout->oobavail +=
			chip->ecc.layout->oobfree[i].length;
	mtd->oobavail = chip->ecc.layout->oobavail;

	/*
	 * Set the number of read / write steps for one page depending on ECC
	 * mode
	 */
	chip->ecc.steps = mtd->writesize / chip->ecc.size;
	if(chip->ecc.steps * chip->ecc.size != mtd->writesize) {
		pr_warning("Invalid ecc parameters\n");
		BUG();
	}
	chip->ecc.total = chip->ecc.steps * chip->ecc.bytes;

	/*
	 * Allow subpage writes up to ecc.steps. Not possible for MLC
	 * FLASH.
	 */
	if (!(chip->options & NAND_NO_SUBPAGE_WRITE) &&
	    !(chip->cellinfo & NAND_CI_CELLTYPE_MSK)) {
		switch(chip->ecc.steps) {
		case 2:
			mtd->subpage_sft = 1;
			break;
		case 4:
		case 8:
			mtd->subpage_sft = 2;
			break;
		}
	}
	chip->subpagesize = mtd->writesize >> mtd->subpage_sft;

	/* Initialize state */
	chip->state = FL_READY;

	/* De-select the device */
	chip->select_chip(mtd, -1);

	/* Invalidate the pagebuffer reference */
	chip->pagebuf = -1;

	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
#ifdef CONFIG_MTD_WRITE
	mtd->erase = nand_erase;
	mtd->write = nand_write;
	mtd->write_oob = nand_write_oob;
#endif
	mtd->read = nand_read;
#ifdef CONFIG_NAND_READ_OOB
	mtd->read_oob = nand_read_oob;
#endif
	mtd->lock = NULL;
	mtd->unlock = NULL;
	mtd->block_isbad = nand_block_isbad;
#ifdef CONFIG_MTD_WRITE
	mtd->block_markbad = nand_block_markbad;
#endif
	/* propagate ecc.layout to mtd_info */
	mtd->ecclayout = chip->ecc.layout;

	/* Check, if we should skip the bad block table scan */
	if (chip->options & NAND_SKIP_BBTSCAN)
		return 0;
#ifdef CONFIG_NAND_BBT
	/* Build bad block table */
	return chip->scan_bbt(mtd);
#else
	return 0;
#endif
}

/**
 * nand_scan - [NAND Interface] Scan for the NAND device
 * @mtd:	MTD device structure
 * @maxchips:	Number of chips to scan for
 *
 * This fills out all the uninitialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 * The mtd->owner field must be set to the module of the caller
 *
 */
int nand_scan(struct mtd_info *mtd, int maxchips)
{
	int ret;

	ret = nand_scan_ident(mtd, maxchips);
	if (!ret)
		ret = nand_scan_tail(mtd);
	return ret;
}

/**
 * nand_release - [NAND Interface] Free resources held by the NAND device
 * @mtd:	MTD device structure
*/
void nand_release(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	/* Deregister the device */
	del_mtd_device(mtd);

	/* Free bad block table memory */
	kfree(chip->bbt);
	if (!(chip->options & NAND_OWN_BUFFERS))
		kfree(chip->buffers);
}

EXPORT_SYMBOL(nand_scan);
EXPORT_SYMBOL(nand_scan_ident);
EXPORT_SYMBOL(nand_scan_tail);
EXPORT_SYMBOL(nand_release);

static int mtd_set_erasebad(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct mtd_info *mtd = container_of(dev, struct mtd_info, class_dev);
	int erasebad;

	erasebad = simple_strtoul(val, NULL, 0);

	if (erasebad && !mtd->allow_erasebad)
		dev_warn(dev, "Allowing to erase bad blocks. This may be dangerous!\n");

	mtd->allow_erasebad = erasebad ? true : false;

	return 0;
}

static const char *mtd_get_bbt_type(struct device_d *dev, struct param_d *p)
{
	struct mtd_info *mtd = container_of(dev, struct mtd_info, class_dev);
	struct nand_chip *chip = mtd->priv;
	const char *str;

	if (!chip->bbt)
		str = "none";
	else if ((chip->bbt_td && chip->bbt_td->pages[0] != -1) ||
				(chip->bbt_md && chip->bbt_md->pages[0] != -1))
		str = "flashbased";
	else
		str = "memorybased";

	return str;
}

int add_mtd_nand_device(struct mtd_info *mtd, char *devname)
{
	int ret;

	ret = add_mtd_device(mtd, devname);
	if (ret)
		return ret;

	if (IS_ENABLED(CONFIG_NAND_ALLOW_ERASE_BAD))
		dev_add_param(&mtd->class_dev, "erasebad", mtd_set_erasebad,
				NULL, 0);

	dev_add_param(&mtd->class_dev, "bbt", NULL, mtd_get_bbt_type, 0);

	return ret;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */
