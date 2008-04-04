/*
 *  drivers/mtd/nand.c
 *
 *  Overview:
 *   This is the generic MTD driver for NAND flash devices. It should be
 *   capable of working with almost all NAND chips currently available.
 *   Basic support for AG-AND chips is provided.
 *
 *	Additional technical information is available on
 *	http://www.linux-mtd.infradead.org/tech/nand.html
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 * 		  2002 Thomas Gleixner (tglx@linutronix.de)
 *
 *  02-08-2004  tglx: support for strange chips, which cannot auto increment
 *		pages on read / read_oob
 *
 *  03-17-2004  tglx: Check ready before auto increment check. Simon Bayes
 *		pointed this out, as he marked an auto increment capable chip
 *		as NOAUTOINCR in the board driver.
 *		Make reads over block boundaries work too
 *
 *  04-14-2004	tglx: first working version for 2k page size chips
 *
 *  05-19-2004  tglx: Basic support for Renesas AG-AND chips
 *
 *  09-24-2004  tglx: add support for hardware controllers (e.g. ECC) shared
 *		among multiple independend devices. Suggestions and initial patch
 *		from Ben Dooks <ben-mtd@fluff.org>
 *
 * Credits:
 *	David Woodhouse for adding multichip support
 *
 *	Aleph One Ltd. and Toby Churchill Ltd. for supporting the
 *	rework for 2K page size chips
 *
 * TODO:
 *	Enable cached programming for 2k page size chips
 *	Check, if mtd->ecctype should be set to MTD_ECC_HW
 *	if we have HW ecc support.
 *	The AG-AND chips have nice features for speed improvement,
 *	which are not supported yet. Read / program 4 pages in one go.
 *
 * $Id: nand_base.c,v 1.126 2004/12/13 11:22:25 lavinen Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/* XXX U-BOOT XXX */
#if 0
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/compatmac.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <asm/io.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif

#endif

#include <common.h>

#include <malloc.h>
#include <watchdog.h>
#include <linux/mtd/compat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <clock.h>

#include <asm/io.h>
#include <errno.h>

#ifdef CONFIG_JFFS2_NAND
#include <jffs2/jffs2.h>
#endif

/* Define default oob placement schemes for large and small page devices */
static struct nand_oobinfo nand_oob_8 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 3,
	.eccpos = {0, 1, 2},
	.oobfree = { {3, 2}, {6, 2} }
};

static struct nand_oobinfo nand_oob_16 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 6,
	.eccpos = {0, 1, 2, 3, 6, 7},
	.oobfree = { {8, 8} }
};

static struct nand_oobinfo nand_oob_64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 38} }
};

/* This is used for padding purposes in nand_write_oob */
static u_char ffchars[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

/*
 * NAND low-level MTD interface functions
 */
static void nand_write_buf(struct nand_chip *this, const u_char *buf, int len);
static void nand_read_buf(struct nand_chip *this, u_char *buf, int len);
static int nand_verify_buf(struct nand_chip *this, const u_char *buf, int len);

static int nand_read (struct nand_chip *this, loff_t from, size_t len, size_t * retlen, u_char * buf);
static int nand_read_ecc (struct nand_chip *this, loff_t from, size_t len,
			  size_t * retlen, u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel);
static int nand_read_oob (struct nand_chip *this, loff_t from, size_t len, size_t * retlen, u_char * buf);
static int nand_write (struct nand_chip *this, loff_t to, size_t len, size_t * retlen, const u_char * buf);
static int nand_write_ecc (struct nand_chip *this, loff_t to, size_t len,
			   size_t * retlen, const u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel);
static int nand_write_oob (struct nand_chip *this, loff_t to, size_t len, size_t * retlen, const u_char *buf);
static int nand_erase (struct nand_chip *this, struct erase_info *instr);
static void nand_sync (struct nand_chip *this);

/* Some internal functions */
static int nand_write_page (struct nand_chip *this, int page, u_char *oob_buf,
		struct nand_oobinfo *oobsel, int mode);
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
static int nand_verify_pages (struct nand_chip *this, int page, int numpages,
	u_char *oob_buf, struct nand_oobinfo *oobsel, int chipnr, int oobmode);
#else
#define nand_verify_pages(...) (0)
#endif

static void nand_release_device (struct nand_chip *this)
{
	this->select_chip(this, -1);	/* De-select the NAND device */
}

/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @param mtd	MTD device structure
 *
 * Default read function for 8bit buswith
 */
static u_char nand_read_byte(struct nand_chip *this)
{
	return readb(this->IO_ADDR_R);
}

/**
 * nand_write_byte - [DEFAULT] write one byte to the chip
 * @param mtd	MTD device structure
 * @param byte	pointer to data byte to write
 *
 * Default write function for 8it buswith
 */
static void nand_write_byte(struct nand_chip *this, u_char byte)
{
	writeb(byte, this->IO_ADDR_W);
}

/**
 * nand_read_byte16 - [DEFAULT] read one byte endianess aware from the chip
 * @param mtd	MTD device structure
 *
 * Default read function for 16bit buswith with
 * endianess conversion
 */
static u_char nand_read_byte16(struct nand_chip *this)
{
	return (u_char) cpu_to_le16(readw(this->IO_ADDR_R));
}

/**
 * nand_write_byte16 - [DEFAULT] write one byte endianess aware to the chip
 * @param mtd	MTD device structure
 * @param byte	pointer to data byte to write
 *
 * Default write function for 16bit buswith with
 * endianess conversion
 */
static void nand_write_byte16(struct nand_chip *this, u_char byte)
{
	writew(le16_to_cpu((u16) byte), this->IO_ADDR_W);
}

/**
 * nand_read_word - [DEFAULT] read one word from the chip
 * @param mtd	MTD device structure
 *
 * Default read function for 16bit buswith without
 * endianess conversion
 */
static u16 nand_read_word(struct nand_chip *this)
{
	return readw(this->IO_ADDR_R);
}

/**
 * nand_write_word - [DEFAULT] write one word to the chip
 * @param mtd	MTD device structure
 * @param word	data word to write
 *
 * Default write function for 16bit buswith without
 * endianess conversion
 */
static void nand_write_word(struct nand_chip *this, u16 word)
{
	writew(word, this->IO_ADDR_W);
}

/**
 * nand_select_chip - [DEFAULT] control CE line
 * @param mtd	MTD device structure
 * @param chip	chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void nand_select_chip(struct nand_chip *this, int chip)
{
	switch(chip) {
	case -1:
		this->hwcontrol(this, NAND_CTL_CLRNCE);
		break;
	case 0:
		this->hwcontrol(this, NAND_CTL_SETNCE);
		break;

	default:
		BUG();
	}
}

/**
 * nand_write_buf - [DEFAULT] write buffer to chip
 * @param mtd	MTD device structure
 * @param buf	data buffer
 * @param len	number of bytes to write
 *
 * Default write function for 8bit buswith
 */
static void nand_write_buf(struct nand_chip *this, const u_char *buf, int len)
{
	int i;

	for (i=0; i<len; i++)
		writeb(buf[i], this->IO_ADDR_W);
}

/**
 * nand_read_buf - [DEFAULT] read chip data into buffer
 * @param mtd	MTD device structure
 * @param buf	buffer to store date
 * @param len	number of bytes to read
 *
 * Default read function for 8bit buswith
 */
static void nand_read_buf(struct nand_chip *this, u_char *buf, int len)
{
	int i;

	for (i=0; i<len; i++)
		buf[i] = readb(this->IO_ADDR_R);
}

/**
 * nand_verify_buf - [DEFAULT] Verify chip data against buffer
 * @param mtd	MTD device structure
 * @param buf	buffer containing the data to compare
 * @param len	number of bytes to compare
 *
 * Default verify function for 8bit buswith
 */
static int nand_verify_buf(struct nand_chip *this, const u_char *buf, int len)
{
	int i;

	for (i=0; i<len; i++)
		if (buf[i] != readb(this->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

/**
 * nand_write_buf16 - [DEFAULT] write buffer to chip
 * @param mtd	MTD device structure
 * @param buf	data buffer
 * @param len	number of bytes to write
 *
 * Default write function for 16bit buswith
 */
static void nand_write_buf16(struct nand_chip *this, const u_char *buf, int len)
{
	int i;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i=0; i<len; i++)
		writew(p[i], this->IO_ADDR_W);

}

/**
 * nand_read_buf16 - [DEFAULT] read chip data into buffer
 * @param mtd	MTD device structure
 * @param buf	buffer to store date
 * @param len	number of bytes to read
 *
 * Default read function for 16bit buswith
 */
static void nand_read_buf16(struct nand_chip *this, u_char *buf, int len)
{
	int i;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i=0; i<len; i++)
		p[i] = readw(this->IO_ADDR_R);
}

/**
 * nand_verify_buf16 - [DEFAULT] Verify chip data against buffer
 * @param mtd	MTD device structure
 * @param buf	buffer containing the data to compare
 * @param len	number of bytes to compare
 *
 * Default verify function for 16bit buswith
 */
static int nand_verify_buf16(struct nand_chip *this, const u_char *buf, int len)
{
	int i;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i=0; i<len; i++)
		if (p[i] != readw(this->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

/**
 * nand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @param mtd	MTD device structure
 * @param ofs	offset from device start
 * @param getchip	0, if the chip is already selected
 *
 * Check, if the block is bad.
 */
static int nand_block_bad(struct nand_chip *this, loff_t ofs, int getchip)
{
	int page, chipnr, res = 0;
	u16 bad;

	if (getchip) {
		page = (int)(ofs >> this->page_shift);
		chipnr = (int)(ofs >> this->chip_shift);

		/* Select the NAND device */
		this->select_chip(this, chipnr);
	} else
		page = (int) ofs;

	if (this->options & NAND_BUSWIDTH_16) {
		this->cmdfunc (this, NAND_CMD_READOOB, this->badblockpos & 0xFE, page & this->pagemask);
		bad = cpu_to_le16(this->read_word(this));
		if (this->badblockpos & 0x1)
			bad >>= 1;
		if ((bad & 0xFF) != 0xff)
			res = 1;
	} else {
		this->cmdfunc (this, NAND_CMD_READOOB, this->badblockpos, page & this->pagemask);
		if (this->read_byte(this) != 0xff)
			res = 1;
	}

	if (getchip) {
		/* Deselect and wake up anyone waiting on the device */
		nand_release_device(this);
	}

	return res;
}

/**
 * nand_default_block_markbad - [DEFAULT] mark a block bad
 * @param mtd	MTD device structure
 * @param ofs	offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
*/
static int nand_default_block_markbad(struct nand_chip *this, loff_t ofs)
{
	u_char buf[2] = {0, 0};
	size_t	retlen;
	int block;

	/* Get block number */
	block = ((int) ofs) >> this->bbt_erase_shift;
	this->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

	/* Do we have a flash based bad block table ? */
	if (this->options & NAND_USE_FLASH_BBT)
		return nand_update_bbt (this, ofs);

	/* We write two bytes, so we dont have to mess with 16 bit access */
	ofs += this->oobsize + (this->badblockpos & ~0x01);
	return nand_write_oob (this, ofs , 2, &retlen, buf);
}

/**
 * nand_check_wp - [GENERIC] check if the chip is write protected
 * @param mtd	MTD device structure
 * Check, if the device is write protected
 *
 * The function expects, that the device is already selected
 */
static int nand_check_wp (struct nand_chip *this)
{
	/* Check the WP bit */
	this->cmdfunc (this, NAND_CMD_STATUS, -1, -1);
	return (this->read_byte(this) & 0x80) ? 0 : 1;
}

/**
 * nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @param mtd	MTD device structure
 * @param ofs	offset from device start
 * @param getchip	0, if the chip is already selected
 * @param allowbbt	1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int nand_block_checkbad (struct nand_chip *this, loff_t ofs, int getchip, int allowbbt)
{
	if (!this->bbt)
		return this->block_bad(this, ofs, getchip);

	/* Return info from the table */
	return nand_isbad_bbt (this, ofs, allowbbt);
}

/**
 * nand_command - [DEFAULT] Send command to NAND device
 * @param mtd	MTD device structure
 * @param command	the command to be sent
 * @param column	the column address for this command, -1 if none
 * @param page_addr	the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small page
 * devices (256/512 Bytes per page)
 */
static void nand_command (struct nand_chip *this, unsigned command, int column, int page_addr)
{
	/* Begin command latch cycle */
	this->hwcontrol(this, NAND_CTL_SETCLE);
	/*
	 * Write out the command to the device.
	 */
	if (command == NAND_CMD_SEQIN) {
		int readcmd;

		if (column >= this->oobblock) {
			/* OOB area */
			column -= this->oobblock;
			readcmd = NAND_CMD_READOOB;
		} else if (column < 256) {
			/* First 256 bytes --> READ0 */
			readcmd = NAND_CMD_READ0;
		} else {
			column -= 256;
			readcmd = NAND_CMD_READ1;
		}
		this->write_byte(this, readcmd);
	}
	this->write_byte(this, command);

	/* Set ALE and clear CLE to start address cycle */
	this->hwcontrol(this, NAND_CTL_CLRCLE);

	if (column != -1 || page_addr != -1) {
		this->hwcontrol(this, NAND_CTL_SETALE);

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;
			this->write_byte(this, column);
		}
		if (page_addr != -1) {
			this->write_byte(this, (unsigned char) (page_addr & 0xff));
			this->write_byte(this, (unsigned char) ((page_addr >> 8) & 0xff));
			/* One more address cycle for devices > 32MiB */
			if (this->chipsize > (32 << 20))
				this->write_byte(this, (unsigned char) ((page_addr >> 16) & 0x0f));
		}
		/* Latch in address */
		this->hwcontrol(this, NAND_CTL_CLRALE);
	}

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
		if (this->dev_ready)
			break;
		udelay(this->chip_delay);
		this->hwcontrol(this, NAND_CTL_SETCLE);
		this->write_byte(this, NAND_CMD_STATUS);
		this->hwcontrol(this, NAND_CTL_CLRCLE);
		while ( !(this->read_byte(this) & 0x40));
		return;

	/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		*/
		if (!this->dev_ready) {
			udelay (this->chip_delay);
			return;
		}
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay (100);
	/* wait until command is processed */
	while (!this->dev_ready(this));
}

/**
 * nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @param mtd	MTD device structure
 * @param command	the command to be sent
 * @param column	the column address for this command, -1 if none
 * @param page_addr	the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page devices
 * We dont have the seperate regions as we have in the small page devices.
 * We must emulate NAND_CMD_READOOB to keep the code compatible.
 *
 */
static void nand_command_lp (struct nand_chip *this, unsigned command, int column, int page_addr)
{
	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += this->oobblock;
		command = NAND_CMD_READ0;
	}


	/* Begin command latch cycle */
	this->hwcontrol(this, NAND_CTL_SETCLE);
	/* Write out the command to the device. */
	this->write_byte(this, command);
	/* End command latch cycle */
	this->hwcontrol(this, NAND_CTL_CLRCLE);

	if (column != -1 || page_addr != -1) {
		this->hwcontrol(this, NAND_CTL_SETALE);

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;
			this->write_byte(this, column & 0xff);
			this->write_byte(this, column >> 8);
		}
		if (page_addr != -1) {
			this->write_byte(this, (unsigned char) (page_addr & 0xff));
			this->write_byte(this, (unsigned char) ((page_addr >> 8) & 0xff));
			/* One more address cycle for devices > 128MiB */
			if (this->chipsize > (128 << 20))
				this->write_byte(this, (unsigned char) ((page_addr >> 16) & 0xff));
		}
		/* Latch in address */
		this->hwcontrol(this, NAND_CTL_CLRALE);
	}

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	*/
	switch (command) {

	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
		return;


	case NAND_CMD_RESET:
		if (this->dev_ready)
			break;
		udelay(this->chip_delay);
		this->hwcontrol(this, NAND_CTL_SETCLE);
		this->write_byte(this, NAND_CMD_STATUS);
		this->hwcontrol(this, NAND_CTL_CLRCLE);
		while ( !(this->read_byte(this) & 0x40));
		return;

	case NAND_CMD_READ0:
		/* Begin command latch cycle */
		this->hwcontrol(this, NAND_CTL_SETCLE);
		/* Write out the start read command */
		this->write_byte(this, NAND_CMD_READSTART);
		/* End command latch cycle */
		this->hwcontrol(this, NAND_CTL_CLRCLE);
		/* Fall through into ready check */

	/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		*/
		if (!this->dev_ready) {
			udelay (this->chip_delay);
			return;
		}
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay (100);
	/* wait until command is processed */
	while (!this->dev_ready(this));
}

static int nand_wait(struct nand_chip *this, int state)
{
	uint64_t timeo, start;

	if (state == FL_ERASING)
 		timeo = 400 * MSECOND;
	else
		timeo = 20 * MSECOND;

	if ((state == FL_ERASING) && (this->options & NAND_IS_AND))
		this->cmdfunc(this, NAND_CMD_STATUS_MULTI, -1, -1);
	else
		this->cmdfunc(this, NAND_CMD_STATUS, -1, -1);

	start = get_time_ns();

	while (1) {
		if (is_timeout(start, timeo)) {
			printf("Timeout!");
			return 0x01;
		}

		if (this->dev_ready) {
			if (this->dev_ready(this))
				break;
		} else {
			if (this->read_byte(this) & NAND_STATUS_READY)
				break;
		}
	}
#ifdef PPCHAMELON_NAND_TIMER_HACK
	reset_timer();
	while (get_timer(0) < 10);
#endif /*  PPCHAMELON_NAND_TIMER_HACK */

	return this->read_byte(this);
}


/**
 * nand_write_page - [GENERIC] write one page
 * @param param mtd		MTD device structure
 * @param param this		NAND chip structure
 * @param param page	 	startpage inside the chip, must be called with (page & this->pagemask)
 * @param param oob_buf	out of band data buffer
 * @param param oobsel	out of band selecttion structre
 * @param param cached	1 = enable cached programming if supported by chip
 *
 * Nand_page_program function is used for write and writev !
 * This function will always program a full page of data
 * If you call it with a non page aligned buffer, you're lost :)
 *
 * Cached programming is not supported yet.
 */
static int nand_write_page (struct nand_chip *this, int page,
	u_char *oob_buf,  struct nand_oobinfo *oobsel, int cached)
{
	int 	i, status;
	u_char	ecc_code[32];
	int	eccmode = oobsel->useecc ? this->eccmode : NAND_ECC_NONE;
	uint  	*oob_config = oobsel->eccpos;
	int	datidx = 0, eccidx = 0, eccsteps = this->eccsteps;
	int	eccbytes = 0;

	/* FIXME: Enable cached programming */
	cached = 0;

	/* Send command to begin auto page programming */
	this->cmdfunc (this, NAND_CMD_SEQIN, 0x00, page);

	/* Write out complete page of data, take care of eccmode */
	switch (eccmode) {
	/* No ecc, write all */
	case NAND_ECC_NONE:
		printk (KERN_WARNING "Writing data without ECC to NAND-FLASH is not recommended\n");
		this->write_buf(this, this->data_poi, this->oobblock);
		break;

	/* Software ecc 3/256, write all */
	case NAND_ECC_SOFT:
		for (; eccsteps; eccsteps--) {
			this->calculate_ecc(this, &this->data_poi[datidx], ecc_code);
			for (i = 0; i < 3; i++, eccidx++)
				oob_buf[oob_config[eccidx]] = ecc_code[i];
			datidx += this->eccsize;
		}
		this->write_buf(this, this->data_poi, this->oobblock);
		break;
	default:
		eccbytes = this->eccbytes;
		for (; eccsteps; eccsteps--) {
			/* enable hardware ecc logic for write */
			this->enable_hwecc(this, NAND_ECC_WRITE);
			this->write_buf(this, &this->data_poi[datidx], this->eccsize);
			this->calculate_ecc(this, &this->data_poi[datidx], ecc_code);
			for (i = 0; i < eccbytes; i++, eccidx++)
				oob_buf[oob_config[eccidx]] = ecc_code[i];
			/* If the hardware ecc provides syndromes then
			 * the ecc code must be written immidiately after
			 * the data bytes (words) */
			if (this->options & NAND_HWECC_SYNDROME)
				this->write_buf(this, ecc_code, eccbytes);
			datidx += this->eccsize;
		}
		break;
	}

	/* Write out OOB data */
	if (this->options & NAND_HWECC_SYNDROME)
		this->write_buf(this, &oob_buf[oobsel->eccbytes], this->oobsize - oobsel->eccbytes);
	else
		this->write_buf(this, oob_buf, this->oobsize);

	/* Send command to actually program the data */
	this->cmdfunc (this, cached ? NAND_CMD_CACHEDPROG : NAND_CMD_PAGEPROG, -1, -1);

	if (!cached) {
		/* call wait ready function */
		status = this->waitfunc (this, FL_WRITING);
		/* See if device thinks it succeeded */
		if (status & 0x01) {
			DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write, page 0x%08x, ", __FUNCTION__, page);
			return -EIO;
		}
	} else {
		/* FIXME: Implement cached programming ! */
		/* wait until cache is ready*/
		/* status = this->waitfunc (this, this, FL_CACHEDRPG); */
	}
	return 0;
}

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
/**
 * nand_verify_pages - [GENERIC] verify the chip contents after a write
 * @param mtd	MTD device structure
 * @param this	NAND chip structure
 * @param page: 	startpage inside the chip, must be called with (page & this->pagemask)
 * @param numpages	number of pages to verify
 * @param oob_buf	out of band data buffer
 * @param oobsel	out of band selecttion structre
 * @param chipnr	number of the current chip
 * @param oobmode	1 = full buffer verify, 0 = ecc only
 *
 * The NAND device assumes that it is always writing to a cleanly erased page.
 * Hence, it performs its internal write verification only on bits that
 * transitioned from 1 to 0. The device does NOT verify the whole page on a
 * byte by byte basis. It is possible that the page was not completely erased
 * or the page is becoming unusable due to wear. The read with ECC would catch
 * the error later when the ECC page check fails, but we would rather catch
 * it early in the page write stage. Better to write no data than invalid data.
 */
static int nand_verify_pages (struct nand_chip *this, int page, int numpages,
	u_char *oob_buf, struct nand_oobinfo *oobsel, int chipnr, int oobmode)
{
	int 	i, j, datidx = 0, oobofs = 0, res = -EIO;
	int	eccsteps = this->eccsteps;
	int	hweccbytes;
	u_char 	oobdata[64];

	hweccbytes = (this->options & NAND_HWECC_SYNDROME) ? (oobsel->eccbytes / eccsteps) : 0;

	/* Send command to read back the first page */
	this->cmdfunc (this, NAND_CMD_READ0, 0, page);

	for(;;) {
		for (j = 0; j < eccsteps; j++) {
			/* Loop through and verify the data */
			if (this->verify_buf(this, &this->data_poi[datidx], this->eccsize)) {
				DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
				goto out;
			}
			datidx += this->eccsize;
			/* Have we a hw generator layout ? */
			if (!hweccbytes)
				continue;
			if (this->verify_buf(this, &this->oob_buf[oobofs], hweccbytes)) {
				DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
				goto out;
			}
			oobofs += hweccbytes;
		}

		/* check, if we must compare all data or if we just have to
		 * compare the ecc bytes
		 */
		if (oobmode) {
			if (this->verify_buf(this, &oob_buf[oobofs], this->oobsize - hweccbytes * eccsteps)) {
				DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write verify, page 0x%08x ", __FUNCTION__, page);
				goto out;
			}
		} else {
			/* Read always, else autoincrement fails */
			this->read_buf(this, oobdata, this->oobsize - hweccbytes * eccsteps);

			if (oobsel->useecc != MTD_NANDECC_OFF && !hweccbytes) {
				int ecccnt = oobsel->eccbytes;

				for (i = 0; i < ecccnt; i++) {
					int idx = oobsel->eccpos[i];
					if (oobdata[idx] != oob_buf[oobofs + idx] ) {
						DEBUG (MTD_DEBUG_LEVEL0,
					       	"%s: Failed ECC write "
						"verify, page 0x%08x, " "%6i bytes were succesful\n", __FUNCTION__, page, i);
						goto out;
					}
				}
			}
		}
		oobofs += this->oobsize - hweccbytes * eccsteps;
		page++;
		numpages--;

		/* Apply delay or wait for ready/busy pin
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		 * Do this also before returning, so the chip is
		 * ready for the next command.
		*/
		if (!this->dev_ready)
			udelay (this->chip_delay);
		else
			while (!this->dev_ready(this));

		/* All done, return happy */
		if (!numpages)
			return 0;


		/* Check, if the chip supports auto page increment */
		if (!NAND_CANAUTOINCR(this))
			this->cmdfunc (this, NAND_CMD_READ0, 0x00, page);
	}
	/*
	 * Terminate the read command. We come here in case of an error
	 * So we must issue a reset command.
	 */
out:
	this->cmdfunc (this, NAND_CMD_RESET, -1, -1);
	return res;
}
#endif

/**
 * nand_read - [MTD Interface] MTD compability function for nand_read_ecc
 * @param mtd	MTD device structure
 * @param from	offset to read from
 * @param len	number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf	the databuffer to put data
 *
 * This function simply calls nand_read_ecc with oob buffer and oobsel = NULL
*/
static int nand_read (struct nand_chip *this, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	return nand_read_ecc (this, from, len, retlen, buf, NULL, NULL);
}


/**
 * nand_read_ecc - [MTD Interface] Read data with ECC
 * @param mtd	MTD device structure
 * @param from	offset to read from
 * @param len	number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf	the databuffer to put data
 * @param oob_buf	filesystem supplied oob data buffer
 * @param oobsel	oob selection structure
 *
 * NAND read with ECC
 */
static int nand_read_ecc (struct nand_chip *this, loff_t from, size_t len,
			  size_t * retlen, u_char * buf, u_char * oob_buf, struct nand_oobinfo *oobsel)
{
	int i, j, col, realpage, page, end, ecc, chipnr, sndcmd = 1;
	int read = 0, oob = 0, ecc_status = 0, ecc_failed = 0;
	u_char *data_poi, *oob_data = oob_buf;
	u_char ecc_calc[32];
	u_char ecc_code[32];
	int eccmode, eccsteps;
	unsigned *oob_config;
	int	datidx;
	int	blockcheck = (1 << (this->phys_erase_shift - this->page_shift)) - 1;
	int	eccbytes;
	int	compareecc = 1;
	int	oobreadlen;


	DEBUG (MTD_DEBUG_LEVEL3, "nand_read_ecc: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Do not allow reads past end of device */
	if ((from + len) > this->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* use userspace supplied oobinfo, if zero */
	if (oobsel == NULL)
		oobsel = &this->oobinfo;

	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE)
		oobsel = this->autooob;

	eccmode = oobsel->useecc ? this->eccmode : NAND_ECC_NONE;
	oob_config = oobsel->eccpos;

	/* Select the NAND device */
	chipnr = (int)(from >> this->chip_shift);
	this->select_chip(this, chipnr);

	/* First we calculate the starting page */
	realpage = (int) (from >> this->page_shift);
	page = realpage & this->pagemask;

	/* Get raw starting column */
	col = from & (this->oobblock - 1);

	end = this->oobblock;
	ecc = this->eccsize;
	eccbytes = this->eccbytes;

	if ((eccmode == NAND_ECC_NONE) || (this->options & NAND_HWECC_SYNDROME))
		compareecc = 0;

	oobreadlen = this->oobsize;
	if (this->options & NAND_HWECC_SYNDROME)
		oobreadlen -= oobsel->eccbytes;

	/* Loop until all data read */
	while (read < len) {

		int aligned = (!col && (len - read) >= end);
		/*
		 * If the read is not page aligned, we have to read into data buffer
		 * due to ecc, else we read into return buffer direct
		 */
		if (aligned)
			data_poi = &buf[read];
		else
			data_poi = this->data_buf;

		/* Check, if we have this page in the buffer
		 *
		 * FIXME: Make it work when we must provide oob data too,
		 * check the usage of data_buf oob field
		 */
		if (realpage == this->pagebuf && !oob_buf) {
			/* aligned read ? */
			if (aligned)
				memcpy (data_poi, this->data_buf, end);
			goto readdata;
		}

		/* Check, if we must send the read command */
		if (sndcmd) {
			this->cmdfunc (this, NAND_CMD_READ0, 0x00, page);
			sndcmd = 0;
		}

		/* get oob area, if we have no oob buffer from fs-driver */
		if (!oob_buf || oobsel->useecc == MTD_NANDECC_AUTOPLACE ||
			oobsel->useecc == MTD_NANDECC_AUTOPL_USR)
			oob_data = &this->data_buf[end];

		eccsteps = this->eccsteps;

		switch (eccmode) {
		case NAND_ECC_NONE: {	/* No ECC, Read in a page */
/* XXX U-BOOT XXX */
#if 0
			static unsigned long lastwhinge = 0;
			if ((lastwhinge / HZ) != (jiffies / HZ)) {
				printk (KERN_WARNING "Reading data from NAND FLASH without ECC is not recommended\n");
				lastwhinge = jiffies;
			}
#else
			puts("Reading data from NAND FLASH without ECC is not recommended\n");
#endif
			this->read_buf(this, data_poi, end);
			break;
		}

		case NAND_ECC_SOFT: /* Software ECC 3/256: Read in a page + oob data */
			this->read_buf(this, data_poi, end);
			for (i = 0, datidx = 0; eccsteps; eccsteps--, i+=3, datidx += ecc)
				this->calculate_ecc(this, &data_poi[datidx], &ecc_calc[i]);
			break;

		default:
			for (i = 0, datidx = 0; eccsteps; eccsteps--, i+=eccbytes, datidx += ecc) {
				this->enable_hwecc(this, NAND_ECC_READ);
				this->read_buf(this, &data_poi[datidx], ecc);

				/* HW ecc with syndrome calculation must read the
				 * syndrome from flash immidiately after the data */
				if (!compareecc) {
					/* Some hw ecc generators need to know when the
					 * syndrome is read from flash */
					this->enable_hwecc(this, NAND_ECC_READSYN);
					this->read_buf(this, &oob_data[i], eccbytes);
					/* We calc error correction directly, it checks the hw
					 * generator for an error, reads back the syndrome and
					 * does the error correction on the fly */
					if (this->correct_data(this, &data_poi[datidx], &oob_data[i], &ecc_code[i]) == -1) {
						DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: "
							"Failed ECC read, page 0x%08x on chip %d\n", page, chipnr);
						ecc_failed++;
					}
				} else {
					this->calculate_ecc(this, &data_poi[datidx], &ecc_calc[i]);
				}
			}
			break;
		}

		/* read oobdata */
		this->read_buf(this, &oob_data[this->oobsize - oobreadlen], oobreadlen);

		/* Skip ECC check, if not requested (ECC_NONE or HW_ECC with syndromes) */
		if (!compareecc)
			goto readoob;

		/* Pick the ECC bytes out of the oob data */
		for (j = 0; j < oobsel->eccbytes; j++)
			ecc_code[j] = oob_data[oob_config[j]];

		/* correct data, if neccecary */
		for (i = 0, j = 0, datidx = 0; i < this->eccsteps; i++, datidx += ecc) {
			ecc_status = this->correct_data(this, &data_poi[datidx], &ecc_code[j], &ecc_calc[j]);

			/* Get next chunk of ecc bytes */
			j += eccbytes;

			/* Check, if we have a fs supplied oob-buffer,
			 * This is the legacy mode. Used by YAFFS1
			 * Should go away some day
			 */
			if (oob_buf && oobsel->useecc == MTD_NANDECC_PLACE) {
				int *p = (int *)(&oob_data[this->oobsize]);
				p[i] = ecc_status;
			}

			if (ecc_status == -1) {
				DEBUG (MTD_DEBUG_LEVEL0, "nand_read_ecc: " "Failed ECC read, page 0x%08x\n", page);
				ecc_failed++;
			}
		}

	readoob:
		/* check, if we have a fs supplied oob-buffer */
		if (oob_buf) {
			/* without autoplace. Legacy mode used by YAFFS1 */
			switch(oobsel->useecc) {
			case MTD_NANDECC_AUTOPLACE:
			case MTD_NANDECC_AUTOPL_USR:
				/* Walk through the autoplace chunks */
				for (i = 0, j = 0; j < this->oobavail; i++) {
					int from = oobsel->oobfree[i][0];
					int num = oobsel->oobfree[i][1];
					memcpy(&oob_buf[oob], &oob_data[from], num);
					j+= num;
				}
				oob += this->oobavail;
				break;
			case MTD_NANDECC_PLACE:
				/* YAFFS1 legacy mode */
				oob_data += this->eccsteps * sizeof (int);
			default:
				oob_data += this->oobsize;
			}
		}
	readdata:
		/* Partial page read, transfer data into fs buffer */
		if (!aligned) {
			for (j = col; j < end && read < len; j++)
				buf[read++] = data_poi[j];
			this->pagebuf = realpage;
		} else
			read += this->oobblock;

		/* Apply delay or wait for ready/busy pin
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		*/
		if (!this->dev_ready)
			udelay (this->chip_delay);
		else
			while (!this->dev_ready(this));

		if (read == len)
			break;

		/* For subsequent reads align to page boundary. */
		col = 0;
		/* Increment page address */
		realpage++;

		page = realpage & this->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			this->select_chip(this, -1);
			this->select_chip(this, chipnr);
		}
		/* Check, if the chip supports auto page increment
		 * or if we have hit a block boundary.
		*/
		if (!NAND_CANAUTOINCR(this) || !(page & blockcheck))
			sndcmd = 1;
	}

	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(this);

	/*
	 * Return success, if no ECC failures, else -EBADMSG
	 * fs driver will take care of that, because
	 * retlen == desired len and result == -EBADMSG
	 */
	*retlen = read;
	return ecc_failed ? -EBADMSG : 0;
}

/**
 * nand_read_oob - [MTD Interface] NAND read out-of-band
 * @param mtd	MTD device structure
 * @param from	offset to read from
 * @param len	number of bytes to read
 * @param retlen	pointer to variable to store the number of read bytes
 * @param buf	the databuffer to put data
 *
 * NAND read out-of-band data from the spare area
 */
static int nand_read_oob (struct nand_chip *this, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	int i, col, page, chipnr;
	int	blockcheck = (1 << (this->phys_erase_shift - this->page_shift)) - 1;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_read_oob: from = 0x%08x, len = %i\n", (unsigned int) from, (int) len);

	/* Shift to get page */
	page = (int)(from >> this->page_shift);
	chipnr = (int)(from >> this->chip_shift);

	/* Mask to get column */
	col = from & (this->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow reads past end of device */
	if ((from + len) > this->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_oob: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}

	/* Select the NAND device */
	this->select_chip(this, chipnr);

	/* Send the read command */
	this->cmdfunc (this, NAND_CMD_READOOB, col, page & this->pagemask);
	/*
	 * Read the data, if we read more than one page
	 * oob data, let the device transfer the data !
	 */
	i = 0;
	while (i < len) {
		int thislen = this->oobsize - col;
		thislen = min_t(int, thislen, len);
		this->read_buf(this, &buf[i], thislen);
		i += thislen;

		/* Apply delay or wait for ready/busy pin
		 * Do this before the AUTOINCR check, so no problems
		 * arise if a chip which does auto increment
		 * is marked as NOAUTOINCR by the board driver.
		*/
		if (!this->dev_ready)
			udelay (this->chip_delay);
		else
			while (!this->dev_ready(this));

		/* Read more ? */
		if (i < len) {
			page++;
			col = 0;

			/* Check, if we cross a chip boundary */
			if (!(page & this->pagemask)) {
				chipnr++;
				this->select_chip(this, -1);
				this->select_chip(this, chipnr);
			}

			/* Check, if the chip supports auto page increment
			 * or if we have hit a block boundary.
			*/
			if (!NAND_CANAUTOINCR(this) || !(page & blockcheck)) {
				/* For subsequent page reads set offset to 0 */
				this->cmdfunc (this, NAND_CMD_READOOB, 0x0, page & this->pagemask);
			}
		}
	}

	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(this);

	/* Return happy */
	*retlen = len;
	return 0;
}

/**
 * nand_read_raw - [GENERIC] Read raw data including oob into buffer
 * @param mtd	MTD device structure
 * @param buf	temporary buffer
 * @param from	offset to read from
 * @param len	number of bytes to read
 * @param ooblen	number of oob data bytes to read
 *
 * Read raw data including oob into buffer
 */
int nand_read_raw (struct nand_chip *this, uint8_t *buf, loff_t from, size_t len, size_t ooblen)
{
	int page = (int) (from >> this->page_shift);
	int chip = (int) (from >> this->chip_shift);
	int sndcmd = 1;
	int cnt = 0;
	int pagesize = this->oobblock + this->oobsize;
	int	blockcheck = (1 << (this->phys_erase_shift - this->page_shift)) - 1;

	/* Do not allow reads past end of device */
	if ((from + len) > this->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_read_raw: Attempt read beyond end of device\n");
		return -EINVAL;
	}

	this->select_chip (this, chip);

	/* Add requested oob length */
	len += ooblen;

	while (len) {
		if (sndcmd)
			this->cmdfunc (this, NAND_CMD_READ0, 0, page & this->pagemask);
		sndcmd = 0;

		this->read_buf (this, &buf[cnt], pagesize);

		len -= pagesize;
		cnt += pagesize;
		page++;

		if (!this->dev_ready)
			udelay (this->chip_delay);
		else
			while (!this->dev_ready(this));

		/* Check, if the chip supports auto page increment */
		if (!NAND_CANAUTOINCR(this) || !(page & blockcheck))
			sndcmd = 1;
	}

	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(this);
	return 0;
}


/**
 * nand_prepare_oobbuf - [GENERIC] Prepare the out of band buffer
 * @param mtd	MTD device structure
 * @param fsbuf	buffer given by fs driver
 * @param oobsel	out of band selection structre
 * @param autoplace	1 = place given buffer into the oob bytes
 * @param numpages	number of pages to prepare
 *
 * Return:
 * 1. Filesystem buffer available and autoplacement is off,
 *    return filesystem buffer
 * 2. No filesystem buffer or autoplace is off, return internal
 *    buffer
 * 3. Filesystem buffer is given and autoplace selected
 *    put data from fs buffer into internal buffer and
 *    retrun internal buffer
 *
 * Note: The internal buffer is filled with 0xff. This must
 * be done only once, when no autoplacement happens
 * Autoplacement sets the buffer dirty flag, which
 * forces the 0xff fill before using the buffer again.
 *
*/
static u_char * nand_prepare_oobbuf (struct nand_chip *this, u_char *fsbuf, struct nand_oobinfo *oobsel,
		int autoplace, int numpages)
{
	int i, len, ofs;

	/* Zero copy fs supplied buffer */
	if (fsbuf && !autoplace)
		return fsbuf;

	/* Check, if the buffer must be filled with ff again */
	if (this->oobdirty) {
		memset (this->oob_buf, 0xff,
			this->oobsize << (this->phys_erase_shift - this->page_shift));
		this->oobdirty = 0;
	}

	/* If we have no autoplacement or no fs buffer use the internal one */
	if (!autoplace || !fsbuf)
		return this->oob_buf;

	/* Walk through the pages and place the data */
	this->oobdirty = 1;
	ofs = 0;
	while (numpages--) {
		for (i = 0, len = 0; len < this->oobavail; i++) {
			int to = ofs + oobsel->oobfree[i][0];
			int num = oobsel->oobfree[i][1];
			memcpy (&this->oob_buf[to], fsbuf, num);
			len += num;
			fsbuf += num;
		}
		ofs += this->oobavail;
	}
	return this->oob_buf;
}

#define NOTALIGNED(x) (x & (this->oobblock-1)) != 0

/**
 * nand_write - [MTD Interface] compability function for nand_write_ecc
 * @param mtd	MTD device structure
 * @param to		offset to write to
 * @param len	number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf	the data to write
 *
 * This function simply calls nand_write_ecc with oob buffer and oobsel = NULL
 *
*/
static int nand_write (struct nand_chip *this, loff_t to, size_t len, size_t * retlen, const u_char * buf)
{
	return (nand_write_ecc (this, to, len, retlen, buf, NULL, NULL));
}

/**
 * nand_write_ecc - [MTD Interface] NAND write with ECC
 * @param mtd	MTD device structure
 * @param to		offset to write to
 * @param len	number of bytes to write
 * @param retlen	pointer to variable to store the number of written bytes
 * @param buf	the data to write
 * @param eccbuf	filesystem supplied oob data buffer
 * @param oobsel	oob selection structure
 *
 * NAND write with ECC
 */
static int nand_write_ecc (struct nand_chip *this, loff_t to, size_t len,
			   size_t * retlen, const u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel)
{
	int startpage, page, ret = -EIO, oob = 0, written = 0, chipnr;
	int autoplace = 0, numpages, totalpages;
	u_char *oobbuf, *bufstart;
	int	ppblock = (1 << (this->phys_erase_shift - this->page_shift));

	DEBUG (MTD_DEBUG_LEVEL3, "nand_write_ecc: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Initialize retlen, in case of early exit */
	*retlen = 0;

	/* Do not allow write past end of device */
	if ((to + len) > this->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: Attempt to write past end of page\n");
		return -EINVAL;
	}

	/* reject writes, which are not page aligned */
	if (NOTALIGNED (to) || NOTALIGNED(len)) {
		printk (KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data\n");
		return -EINVAL;
	}

	/* Calculate chipnr */
	chipnr = (int)(to >> this->chip_shift);
	/* Select the NAND device */
	this->select_chip(this, chipnr);

	/* Check, if it is write protected */
	if (nand_check_wp(this))
		goto out;

	/* if oobsel is NULL, use chip defaults */
	if (oobsel == NULL)
		oobsel = &this->oobinfo;

	/* Autoplace of oob data ? Use the default placement scheme */
	if (oobsel->useecc == MTD_NANDECC_AUTOPLACE) {
		oobsel = this->autooob;
		autoplace = 1;
	}
	if (oobsel->useecc == MTD_NANDECC_AUTOPL_USR)
		autoplace = 1;

	/* Setup variables and oob buffer */
	totalpages = len >> this->page_shift;
	page = (int) (to >> this->page_shift);
	/* Invalidate the page cache, if we write to the cached page */
	if (page <= this->pagebuf && this->pagebuf < (page + totalpages))
		this->pagebuf = -1;

	/* Set it relative to chip */
	page &= this->pagemask;
	startpage = page;
	/* Calc number of pages we can write in one go */
	numpages = min (ppblock - (startpage  & (ppblock - 1)), totalpages);
	oobbuf = nand_prepare_oobbuf (this, eccbuf, oobsel, autoplace, numpages);
	bufstart = (u_char *)buf;

	/* Loop until all data is written */
	while (written < len) {

		this->data_poi = (u_char*) &buf[written];
		/* Write one page. If this is the last page to write
		 * or the last page in this block, then use the
		 * real pageprogram command, else select cached programming
		 * if supported by the chip.
		 */
		ret = nand_write_page (this, page, &oobbuf[oob], oobsel, (--numpages > 0));
		if (ret) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: write_page failed %d\n", ret);
			goto out;
		}
		/* Next oob page */
		oob += this->oobsize;
		/* Update written bytes count */
		written += this->oobblock;
		if (written == len)
			goto cmp;

		/* Increment page address */
		page++;

		/* Have we hit a block boundary ? Then we have to verify and
		 * if verify is ok, we have to setup the oob buffer for
		 * the next pages.
		*/
		if (!(page & (ppblock - 1))){
			int ofs;
			this->data_poi = bufstart;
			ret = nand_verify_pages (this, startpage,
				page - startpage,
				oobbuf, oobsel, chipnr, (eccbuf != NULL));
			if (ret) {
				DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: verify_pages failed %d\n", ret);
				goto out;
			}
			*retlen = written;
			bufstart = (u_char*) &buf[written];

			ofs = autoplace ? this->oobavail : this->oobsize;
			if (eccbuf)
				eccbuf += (page - startpage) * ofs;
			totalpages -= page - startpage;
			numpages = min (totalpages, ppblock);
			page &= this->pagemask;
			startpage = page;
			oob = 0;
			this->oobdirty = 1;
			oobbuf = nand_prepare_oobbuf (this, eccbuf, oobsel,
					autoplace, numpages);
			/* Check, if we cross a chip boundary */
			if (!page) {
				chipnr++;
				this->select_chip(this, -1);
				this->select_chip(this, chipnr);
			}
		}
	}
	/* Verify the remaining pages */
cmp:
	this->data_poi = bufstart;
 	ret = nand_verify_pages (this, startpage, totalpages,
		oobbuf, oobsel, chipnr, (eccbuf != NULL));
	if (!ret)
		*retlen = written;
	else
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_ecc: verify_pages failed %d\n", ret);

out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(this);

	return ret;
}


/**
 * nand_write_oob - [MTD Interface] NAND write out-of-band
 * @param param mtd	MTD device structure
 * @param param to		offset to write to
 * @param param len	number of bytes to write
 * @param param retlen	pointer to variable to store the number of written bytes
 * @param buf	the data to write
 *
 * NAND write out-of-band
 */
static int nand_write_oob (struct nand_chip *this, loff_t to, size_t len, size_t * retlen, const u_char * buf)
{
	int column, page, status, ret = -EIO, chipnr;

	DEBUG (MTD_DEBUG_LEVEL3, "nand_write_oob: to = 0x%08x, len = %i\n", (unsigned int) to, (int) len);

	/* Shift to get page */
	page = (int) (to >> this->page_shift);
	chipnr = (int) (to >> this->chip_shift);

	/* Mask to get column */
	column = to & (this->oobsize - 1);

	/* Initialize return length value */
	*retlen = 0;

	/* Do not allow write past end of page */
	if ((column + len) > this->oobsize) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: Attempt to write past end of page\n");
		return -EINVAL;
	}

	/* Select the NAND device */
	this->select_chip(this, chipnr);

	/* Reset the chip. Some chips (like the Toshiba TC5832DC found
	   in one of my DiskOnChip 2000 test units) will clear the whole
	   data page too if we don't do this. I have no clue why, but
	   I seem to have 'fixed' it in the doc2000 driver in
	   August 1999.  dwmw2. */
	this->cmdfunc(this, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (nand_check_wp(this))
		goto out;

	/* Invalidate the page cache, if we write to the cached page */
	if (page == this->pagebuf)
		this->pagebuf = -1;

	if (NAND_MUST_PAD(this)) {
		/* Write out desired data */
		this->cmdfunc (this, NAND_CMD_SEQIN, this->oobblock, page & this->pagemask);
		/* prepad 0xff for partial programming */
		this->write_buf(this, ffchars, column);
		/* write data */
		this->write_buf(this, buf, len);
		/* postpad 0xff for partial programming */
		this->write_buf(this, ffchars, this->oobsize - (len+column));
	} else {
		/* Write out desired data */
		this->cmdfunc (this, NAND_CMD_SEQIN, this->oobblock + column, page & this->pagemask);
		/* write data */
		this->write_buf(this, buf, len);
	}
	/* Send command to program the OOB data */
	this->cmdfunc (this, NAND_CMD_PAGEPROG, -1, -1);

	status = this->waitfunc (this, FL_WRITING);

	/* See if device thinks it succeeded */
	if (status & 0x01) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: " "Failed write, page 0x%08x\n", page);
		ret = -EIO;
		goto out;
	}
	/* Return happy */
	*retlen = len;

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	/* Send command to read back the data */
	this->cmdfunc (this, NAND_CMD_READOOB, column, page & this->pagemask);

	if (this->verify_buf(this, buf, len)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_write_oob: " "Failed write verify, page 0x%08x\n", page);
		ret = -EIO;
		goto out;
	}
#endif
	ret = 0;
out:
	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(this);

	return ret;
}

/**
 * single_erease_cmd - [GENERIC] NAND standard block erase command function
 * @param param this	MTD device structure
 * @param param page	the page address of the block which will be erased
 *
 * Standard erase command for NAND chips
 */
static void single_erase_cmd (struct nand_chip *this, int page)
{
	/* Send commands to erase a block */
	this->cmdfunc (this, NAND_CMD_ERASE1, -1, page);
	this->cmdfunc (this, NAND_CMD_ERASE2, -1, -1);
}

/**
 * multi_erease_cmd - [GENERIC] AND specific block erase command function
 * @param mtd	MTD device structure
 * @param page	the page address of the block which will be erased
 *
 * AND multi block erase command function
 * Erase 4 consecutive blocks
 */
static void multi_erase_cmd (struct nand_chip *this, int page)
{
	/* Send commands to erase a block */
	this->cmdfunc (this, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (this, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (this, NAND_CMD_ERASE1, -1, page++);
	this->cmdfunc (this, NAND_CMD_ERASE1, -1, page);
	this->cmdfunc (this, NAND_CMD_ERASE2, -1, -1);
}

/**
 * nand_erase - [MTD Interface] erase block(s)
 * @param param mtd	MTD device structure
 * @param param instr	erase instruction
 *
 * Erase one ore more blocks
 */
static int nand_erase (struct nand_chip *this, struct erase_info *instr)
{
	return nand_erase_nand (this, instr, 0);
}

/**
 * nand_erase_intern - [NAND Interface] erase block(s)
 * @param param mtd	MTD device structure
 * @param param instr	erase instruction
 * @param param allowbbt	allow erasing the bbt area
 *
 * Erase one ore more blocks
 */
int nand_erase_nand (struct nand_chip *this, struct erase_info *instr, int allowbbt)
{
	int page, len, status, pages_per_block, ret, chipnr;

	DEBUG (MTD_DEBUG_LEVEL3,
	       "nand_erase: start = 0x%08x, len = %i\n", (unsigned int) instr->addr, (unsigned int) instr->len);

	/* Start address must align on block boundary */
	if (instr->addr & ((1 << this->phys_erase_shift) - 1)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Unaligned address\n");
		return -EINVAL;
	}

	/* Length must align on block boundary */
	if (instr->len & ((1 << this->phys_erase_shift) - 1)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Length not block aligned\n");
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if ((instr->len + instr->addr) > this->size) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Erase past end of device\n");
		return -EINVAL;
	}

	instr->fail_addr = 0xffffffff;

	/* Shift to get first page */
	page = (int) (instr->addr >> this->page_shift);
	chipnr = (int) (instr->addr >> this->chip_shift);

	/* Calculate pages in each block */
	pages_per_block = 1 << (this->phys_erase_shift - this->page_shift);

	/* Select the NAND device */
	this->select_chip(this, chipnr);

	/* Check the WP bit */
	/* Check, if it is write protected */
	if (nand_check_wp(this)) {
		DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: Device is write protected!!!\n");
		instr->state = MTD_ERASE_FAILED;
		goto erase_exit;
	}

	/* Loop through the pages */
	len = instr->len;

	instr->state = MTD_ERASING;

	while (len) {
#ifndef NAND_ALLOW_ERASE_ALL
		/* Check if we have a bad block, we do not erase bad blocks ! */
		if (nand_block_checkbad(this, ((loff_t) page) << this->page_shift, 0, allowbbt)) {
			printk (KERN_WARNING "nand_erase: attempt to erase a bad block at page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
#endif
		/* Invalidate the page cache, if we erase the block which contains
		   the current cached page */
		if (page <= this->pagebuf && this->pagebuf < (page + pages_per_block))
			this->pagebuf = -1;

		this->erase_cmd (this, page & this->pagemask);

		status = this->waitfunc (this, FL_ERASING);

		/* See if block erase succeeded */
		if (status & 0x01) {
			DEBUG (MTD_DEBUG_LEVEL0, "nand_erase: " "Failed erase, page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = (page << this->page_shift);
			goto erase_exit;
		}

		/* Increment page address and decrement length */
		len -= (1 << this->phys_erase_shift);
		page += pages_per_block;

		/* Check, if we cross a chip boundary */
		if (len && !(page & this->pagemask)) {
			chipnr++;
			this->select_chip(this, -1);
			this->select_chip(this, chipnr);
		}
	}
	instr->state = MTD_ERASE_DONE;

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/* Deselect and wake up anyone waiting on the device */
	nand_release_device(this);

	/* Return more or less happy */
	return ret;
}

/**
 * nand_sync - [MTD Interface] sync
 * @param param this	MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void nand_sync (struct nand_chip *this)
{
	DEBUG (MTD_DEBUG_LEVEL3, "nand_sync: called\n");

	/* Release it and go back */
	nand_release_device (this);
}


/**
 * nand_block_isbad - [MTD Interface] Check whether the block at the given offset is bad
 * @param param mtd	MTD device structure
 * @param param ofs	offset relative to mtd start
 */
static int nand_block_isbad (struct nand_chip *this, loff_t ofs)
{
	/* Check for invalid offset */
	if (ofs > this->size)
		return -EINVAL;

	return nand_block_checkbad (this, ofs, 1, 0);
}

/**
 * nand_block_markbad - [MTD Interface] Mark the block at the given offset as bad
 * @param param mtd	MTD device structure
 * @param param ofs	offset relative to mtd start
 */
static int nand_block_markbad (struct nand_chip *this, loff_t ofs)
{
	int ret;

	if ((ret = nand_block_isbad(this, ofs))) {
		/* If it was bad already, return success and do nothing. */
		if (ret > 0)
			return 0;
		return ret;
	}

	return this->block_markbad(this, ofs);
}

/**
 * nand_scan - [NAND Interface] Scan for the NAND device
 * @param param mtd	MTD device structure
 * @param param maxchips	Number of chips to scan for
 *
 * This fills out all the not initialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values. Buffers are allocated if
 * they are not provided by the board driver
 *
 */
int nand_scan (struct nand_chip *this, int maxchips)
{
	int i, j, nand_maf_id, nand_dev_id, busw;

	/* Get buswidth to select the correct functions*/
	busw = this->options & NAND_BUSWIDTH_16;

	/* check for proper chip_delay setup, set 20us if not */
	if (!this->chip_delay)
		this->chip_delay = 20;

	/* check, if a user supplied command function given */
	if (this->cmdfunc == NULL)
		this->cmdfunc = nand_command;

	/* check, if a user supplied wait function given */
	if (this->waitfunc == NULL)
		this->waitfunc = nand_wait;

	if (!this->select_chip)
		this->select_chip = nand_select_chip;
	if (!this->write_byte)
		this->write_byte = busw ? nand_write_byte16 : nand_write_byte;
	if (!this->read_byte)
		this->read_byte = busw ? nand_read_byte16 : nand_read_byte;
	if (!this->write_word)
		this->write_word = nand_write_word;
	if (!this->read_word)
		this->read_word = nand_read_word;
	if (!this->block_bad)
		this->block_bad = nand_block_bad;
	if (!this->block_markbad)
		this->block_markbad = nand_default_block_markbad;
	if (!this->write_buf)
		this->write_buf = busw ? nand_write_buf16 : nand_write_buf;
	if (!this->read_buf)
		this->read_buf = busw ? nand_read_buf16 : nand_read_buf;
	if (!this->verify_buf)
		this->verify_buf = busw ? nand_verify_buf16 : nand_verify_buf;
	if (!this->scan_bbt)
		this->scan_bbt = nand_default_bbt;

	/* Select the device */
	this->select_chip(this, 0);

	/* Send the command for reading device ID */
	this->cmdfunc (this, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	nand_maf_id = this->read_byte(this);
	nand_dev_id = this->read_byte(this);

	/* Print and store flash device information */
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {

		if (nand_dev_id != nand_flash_ids[i].id)
			continue;

		if (!this->name) this->name = nand_flash_ids[i].name;
		this->chipsize = nand_flash_ids[i].chipsize << 20;

		/* New devices have all the information in additional id bytes */
		if (!nand_flash_ids[i].pagesize) {
			int extid;
			/* The 3rd id byte contains non relevant data ATM */
			extid = this->read_byte(this);
			/* The 4th id byte is the important one */
			extid = this->read_byte(this);
			/* Calc pagesize */
			this->oobblock = 1024 << (extid & 0x3);
			extid >>= 2;
			/* Calc oobsize */
			this->oobsize = (8 << (extid & 0x01)) * (this->oobblock / 512);
			extid >>= 2;
			/* Calc blocksize. Blocksize is multiples of 64KiB */
			this->erasesize = (64 * 1024)  << (extid & 0x03);
			extid >>= 2;
			/* Get buswidth information */
			busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;

		} else {
			/* Old devices have this data hardcoded in the
			 * device id table */
			this->erasesize = nand_flash_ids[i].erasesize;
			this->oobblock = nand_flash_ids[i].pagesize;
			this->oobsize = this->oobblock / 32;
			busw = nand_flash_ids[i].options & NAND_BUSWIDTH_16;
		}

		/* Check, if buswidth is correct. Hardware drivers should set
		 * this correct ! */
		if (busw != (this->options & NAND_BUSWIDTH_16)) {
			printk (KERN_INFO "NAND device: Manufacturer ID:"
				" 0x%02x, Chip ID: 0x%02x (%s %s)\n", nand_maf_id, nand_dev_id,
				nand_manuf_ids[i].name , this->name);
			printk (KERN_WARNING
				"NAND bus width %d instead %d bit\n",
					(this->options & NAND_BUSWIDTH_16) ? 16 : 8,
					busw ? 16 : 8);
			this->select_chip(this, -1);
			return 1;
		}

		/* Calculate the address shift from the page size */
		this->page_shift = ffs(this->oobblock) - 1;
		this->bbt_erase_shift = this->phys_erase_shift = ffs(this->erasesize) - 1;
		this->chip_shift = ffs(this->chipsize) - 1;

		/* Set the bad block position */
		this->badblockpos = this->oobblock > 512 ?
			NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

		/* Get chip options, preserve non chip based options */
		this->options &= ~NAND_CHIPOPTIONS_MSK;
		this->options |= nand_flash_ids[i].options & NAND_CHIPOPTIONS_MSK;
		/* Set this as a default. Board drivers can override it, if neccecary */
		this->options |= NAND_NO_AUTOINCR;
		/* Check if this is a not a samsung device. Do not clear the options
		 * for chips which are not having an extended id.
		 */
		if (nand_maf_id != NAND_MFR_SAMSUNG && !nand_flash_ids[i].pagesize)
			this->options &= ~NAND_SAMSUNG_LP_OPTIONS;

		/* Check for AND chips with 4 page planes */
		if (this->options & NAND_4PAGE_ARRAY)
			this->erase_cmd = multi_erase_cmd;
		else
			this->erase_cmd = single_erase_cmd;

		/* Do not replace user supplied command function ! */
		if (this->oobblock > 512 && this->cmdfunc == nand_command)
			this->cmdfunc = nand_command_lp;

		/* Try to identify manufacturer */
		for (j = 0; nand_manuf_ids[j].id != 0x0; j++) {
			if (nand_manuf_ids[j].id == nand_maf_id)
				break;
		}
		break;
	}

	if (!nand_flash_ids[i].name) {
#ifndef CFG_NAND_QUIET_TEST
		printk (KERN_WARNING "No NAND device found!!!\n");
#endif
		this->select_chip(this, -1);
		return 1;
	}

	for (i=1; i < maxchips; i++) {
		this->select_chip(this, i);

		/* Send the command for reading device ID */
		this->cmdfunc (this, NAND_CMD_READID, 0x00, -1);

		/* Read manufacturer and device IDs */
		if (nand_maf_id != this->read_byte(this) ||
		    nand_dev_id != this->read_byte(this))
			break;
	}
	if (i > 1)
		printk(KERN_INFO "%d NAND chips detected\n", i);

	/* Allocate buffers, if neccecary */
	if (!this->oob_buf) {
		size_t len;
		len = this->oobsize << (this->phys_erase_shift - this->page_shift);
		this->oob_buf = kmalloc (len, GFP_KERNEL);
		if (!this->oob_buf) {
			printk (KERN_ERR "nand_scan(): Cannot allocate oob_buf\n");
			return -ENOMEM;
		}
		this->options |= NAND_OOBBUF_ALLOC;
	}

	if (!this->data_buf) {
		size_t len;
		len = this->oobblock + this->oobsize;
		this->data_buf = kmalloc (len, GFP_KERNEL);
		if (!this->data_buf) {
			if (this->options & NAND_OOBBUF_ALLOC)
				kfree (this->oob_buf);
			printk (KERN_ERR "nand_scan(): Cannot allocate data_buf\n");
			return -ENOMEM;
		}
		this->options |= NAND_DATABUF_ALLOC;
	}

	/* Store the number of chips and calc total size for this */
	this->numchips = i;
	this->size = i * this->chipsize;
	/* Convert chipsize to number of pages per chip -1. */
	this->pagemask = (this->chipsize >> this->page_shift) - 1;
	/* Preset the internal oob buffer */
	memset(this->oob_buf, 0xff, this->oobsize << (this->phys_erase_shift - this->page_shift));

	/* If no default placement scheme is given, select an
	 * appropriate one */
	if (!this->autooob) {
		/* Select the appropriate default oob placement scheme for
		 * placement agnostic filesystems */
		switch (this->oobsize) {
		case 8:
			this->autooob = &nand_oob_8;
			break;
		case 16:
			this->autooob = &nand_oob_16;
			break;
		case 64:
			this->autooob = &nand_oob_64;
			break;
		default:
			printk (KERN_WARNING "No oob scheme defined for oobsize %d\n",
				this->oobsize);
/*			BUG(); */
		}
	}

	/* The number of bytes available for the filesystem to place fs dependend
	 * oob data */
	if (this->options & NAND_BUSWIDTH_16) {
		this->oobavail = this->oobsize - (this->autooob->eccbytes + 2);
		if (this->autooob->eccbytes & 0x01)
			this->oobavail--;
	} else
		this->oobavail = this->oobsize - (this->autooob->eccbytes + 1);

	/*
	 * check ECC mode, default to software
	 * if 3byte/512byte hardware ECC is selected and we have 256 byte pagesize
	 * fallback to software ECC
	*/
	this->eccsize = 256;	/* set default eccsize */
	this->eccbytes = 3;

	switch (this->eccmode) {
	case NAND_ECC_HW12_2048:
		if (this->oobblock < 2048) {
			printk(KERN_WARNING "2048 byte HW ECC not possible on %d byte page size, fallback to SW ECC\n",
			       this->oobblock);
			this->eccmode = NAND_ECC_SOFT;
			this->calculate_ecc = nand_calculate_ecc;
			this->correct_data = nand_correct_data;
		} else
			this->eccsize = 2048;
		break;

	case NAND_ECC_HW3_512:
	case NAND_ECC_HW6_512:
	case NAND_ECC_HW8_512:
		if (this->oobblock == 256) {
			printk (KERN_WARNING "512 byte HW ECC not possible on 256 Byte pagesize, fallback to SW ECC \n");
			this->eccmode = NAND_ECC_SOFT;
			this->calculate_ecc = nand_calculate_ecc;
			this->correct_data = nand_correct_data;
		} else
			this->eccsize = 512; /* set eccsize to 512 */
		break;

	case NAND_ECC_HW3_256:
		break;

	case NAND_ECC_NONE:
		printk (KERN_WARNING "NAND_ECC_NONE selected by board driver. This is not recommended !!\n");
		this->eccmode = NAND_ECC_NONE;
		break;

	case NAND_ECC_SOFT:
		this->calculate_ecc = nand_calculate_ecc;
		this->correct_data = nand_correct_data;
		break;

	default:
		printk (KERN_WARNING "Invalid NAND_ECC_MODE %d\n", this->eccmode);
/*		BUG(); */
	}

	/* Check hardware ecc function availability and adjust number of ecc bytes per
	 * calculation step
	*/
	switch (this->eccmode) {
	case NAND_ECC_HW12_2048:
		this->eccbytes += 4;
	case NAND_ECC_HW8_512:
		this->eccbytes += 2;
	case NAND_ECC_HW6_512:
		this->eccbytes += 3;
	case NAND_ECC_HW3_512:
	case NAND_ECC_HW3_256:
		if (this->calculate_ecc && this->correct_data && this->enable_hwecc)
			break;
		printk (KERN_WARNING "No ECC functions supplied, Hardware ECC not possible\n");
/*		BUG();	*/
	}

	this->eccsize = this->eccsize;

	/* Set the number of read / write steps for one page to ensure ECC generation */
	switch (this->eccmode) {
	case NAND_ECC_HW12_2048:
		this->eccsteps = this->oobblock / 2048;
		break;
	case NAND_ECC_HW3_512:
	case NAND_ECC_HW6_512:
	case NAND_ECC_HW8_512:
		this->eccsteps = this->oobblock / 512;
		break;
	case NAND_ECC_HW3_256:
	case NAND_ECC_SOFT:
		this->eccsteps = this->oobblock / 256;
		break;

	case NAND_ECC_NONE:
		this->eccsteps = 1;
		break;
	}

/* XXX U-BOOT XXX */
#if 0
	/* Initialize state, waitqueue and spinlock */
	this->state = FL_READY;
	init_waitqueue_head (&this->wq);
	spin_lock_init (&this->chip_lock);
#endif

	/* De-select the device */
	this->select_chip(this, -1);

	/* Invalidate the pagebuffer reference */
	this->pagebuf = -1;

	/* Fill in remaining MTD driver data */
	this->type = MTD_NANDFLASH;
	this->flags = MTD_CAP_NANDFLASH | MTD_ECC;
	this->ecctype = MTD_ECC_SW;
	this->erase = nand_erase;
	this->read = nand_read;
	this->write = nand_write;
	this->read_ecc = nand_read_ecc;
	this->write_ecc = nand_write_ecc;
	this->read_oob = nand_read_oob;
	this->write_oob = nand_write_oob;
/* XXX U-BOOT XXX */
#if 0
	this->readv = NULL;
	this->writev = nand_writev;
	this->writev_ecc = nand_writev_ecc;
#endif
	this->sync = nand_sync;
/* XXX U-BOOT XXX */
#if 0
	this->lock = NULL;
	this->unlock = NULL;
	this->suspend = NULL;
	this->resume = NULL;
#endif
	this->block_isbad = nand_block_isbad;
	this->block_markbad = nand_block_markbad;

	/* and make the autooob the default one */
	memcpy(&this->oobinfo, this->autooob, sizeof(this->oobinfo));
/* XXX U-BOOT XXX */
#if 0
	this->owner = THIS_MODULE;
#endif
	/* Build bad block table */
	return this->scan_bbt (this);
}

/**
 * nand_release - [NAND Interface] Free resources held by the NAND device
 * @param param mtd	MTD device structure
 */
void nand_release (struct nand_chip *this)
{

#ifdef CONFIG_MTD_PARTITIONS
	/* Deregister partitions */
	del_mtd_partitions (this);
#endif
	/* Deregister the device */
/* XXX U-BOOT XXX */
#if 0
	del_mtd_device (this);
#endif
	/* Free bad block table memory, if allocated */
	if (this->bbt)
		kfree (this->bbt);
	/* Buffer allocated by nand_scan ? */
	if (this->options & NAND_OOBBUF_ALLOC)
		kfree (this->oob_buf);
	/* Buffer allocated by nand_scan ? */
	if (this->options & NAND_DATABUF_ALLOC)
		kfree (this->data_buf);
}

