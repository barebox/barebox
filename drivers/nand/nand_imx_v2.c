/*
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <asm/arch/imx-nand.h>
#include <asm/arch/imx-regs.h>
#include <asm/io.h>
#include <errno.h>

/*
 * Addresses for NFC registers
 */
#define MAIN_AREA0      		(host->regs + 0x000)
#define MAIN_AREA1      		(host->regs + 0x200)
#define SPARE_AREA0     		(host->regs + 0x1000)
#define NFC_BUF_SIZE            	(host->regs + 0x1E00)
#define NFC_BUF_ADDR            	(host->regs + 0x1E04)
#define NFC_FLASH_ADDR          	(host->regs + 0x1E06)
#define NFC_FLASH_CMD           	(host->regs + 0x1E08)
#define NFC_CONFIG              	(host->regs + 0x1E0A)
#define NFC_ECC_STATUS_RESULT		(host->regs + 0x1E0C)
#define NFC_ECC_STATUS_RESULT_1		(host->regs + 0x1E0C)
#define NFC_ECC_STATUS_RESULT_2		(host->regs + 0x1E0E)
#define NFC_SPAS			(host->regs + 0x1E10)
#define NFC_WRPROT              	(host->regs + 0x1E12)
#define NFC_UNLOCKSTART_BLKADDR  	(host->regs + 0x1E20)
#define NFC_UNLOCKEND_BLKADDR    	(host->regs + 0x1E22)
#define NFC_UNLOCKSTART_BLKADDR1 	(host->regs + 0x1E24)
#define NFC_UNLOCKEND_BLKADDR1   	(host->regs + 0x1E26)
#define NFC_UNLOCKSTART_BLKADDR2 	(host->regs + 0x1E28)
#define NFC_UNLOCKEND_BLKADDR2   	(host->regs + 0x1E2A)
#define NFC_UNLOCKSTART_BLKADDR3 	(host->regs + 0x1E2C)
#define NFC_UNLOCKEND_BLKADDR3   	(host->regs + 0x1E2E)
#define NFC_NF_WRPRST            	(host->regs + 0x1E18)
#define NFC_CONFIG1              	(host->regs + 0x1E1A)
#define NFC_CONFIG2              	(host->regs + 0x1E1C)

/*
 * Set INT to 0, Set 1 to specific operation bit, rest to 0 in LAUNCH_NFC Register for
 * Specific operation
 */
#define NFC_CMD            		0x1
#define NFC_ADDR           		0x2
#define NFC_INPUT          		0x4
#define NFC_OUTPUT         		0x8
#define NFC_ID             		0x10
#define NFC_STATUS         		0x20

/* Bit Definitions */
#define NFC_OPS_STAT			(1 << 15)
#define NFC_SP_EN           		(1 << 2)
#define NFC_ECC_EN          		(1 << 3)
#define NFC_INT_MSK         		(1 << 4)
#define NFC_BIG             		(1 << 5)
#define NFC_RST             		(1 << 6)
#define NFC_CE              		(1 << 7)
#define NFC_ONE_CYCLE       		(1 << 8)
#define NFC_BLS_LOCKED			0
#define NFC_BLS_LOCKED_DEFAULT		1
#define NFC_BLS_UNLCOKED		2
#define NFC_WPC_LOCK_TIGHT		1
#define NFC_WPC_LOCK			(1 << 1)
#define NFC_WPC_UNLOCK			(1 << 2)
#define NFC_FLASH_ADDR_SHIFT 		0
#define NFC_UNLOCK_END_ADDR_SHIFT	0

#define NFC_ECC_MODE_4    		 (1<<0)
#define NFC_ECC_MODE_8			 ~(1<<0)
#define NFC_SPAS_16			 8
#define NFC_SPAS_64			 32
#define NFC_SPAS_128			 64
#define NFC_SPAS_218			 109

static inline void raw_write(unsigned long val, void *reg)
{
	writew(val, reg);
	debug("wr: 0x%08x = 0x%08x\n", reg, val);
}

static inline unsigned long raw_read(void *reg)
{
	unsigned long val = readw(reg);

	debug("rd: 0x%08x = 0x%08x\n", reg, val);

	return val;
}

struct imx_nand_host {
	struct mtd_info mtd;
	struct nand_chip nand;
	struct device_d *dev;
	void __iomem *regs;

	int status_request;
	u16 col_addr;
};

/*
 * Define delays in microsec for NAND device operations
 */
#define TROP_US_DELAY   2000

static u8 *data_buf;
static u8 *oob_buf;

/*
 * OOB placement block for use with hardware ecc generation
 */
static struct nand_ecclayout nand_hw_eccoob_512 = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobavail = 4,
	.oobfree = {{0, 4}}
};

static struct nand_ecclayout nand_hw_eccoob_2k = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobavail = 4,
	.oobfree = {{2, 4}}
};

static struct nand_ecclayout nand_hw_eccoob_4k = {
	.eccbytes = 9,
	.eccpos = {7, 8, 9, 10, 11, 12, 13, 14, 15},
	.oobavail = 4,
	.oobfree = {{2, 4}}
};

/*
 * @defgroup NAND_MTD NAND Flash MTD Driver for MXC processors
 */

/*
 * @file mxc_nd2.c
 *
 * @brief This file contains the hardware specific layer for NAND Flash on
 * MXC processor
 *
 * @ingroup NAND_MTD
 */

static void memcpy32(void *trg, const void *src, int size)
{
	int i;
	unsigned int *t = trg;
	unsigned const int *s = src;

	if ((ulong)trg & 0x3 || (ulong)src & 0x3 || size & 0x3)
		while(1);

	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

/*
 * Functions to transfer data to/from spare erea.
 */
static void
copy_spare(struct mtd_info *mtd, void *pbuf, void *pspare, int len, int bfrom)
{
	u16 i, j;
	u16 m = mtd->oobsize;
	u16 n = mtd->writesize >> 9;
	u8 *d = (u8 *) pbuf;
	u8 *s = (u8 *) pspare;

	j = (m / n >> 1) << 1;

	if (bfrom) {
		for (i = 0; i < n - 1; i++)
			memcpy32(&d[i * j], &s[i * 64], j);

		/* the last section */
		memcpy32(&d[i * j], &s[i * 64], len - i * j);
	} else {
		for (i = 0; i < n - 1; i++)
			memcpy32(&s[i * 64], &d[i * j], j);

		/* the last section */
		memcpy32(&s[i * 64], &d[i * j], len - i * j);
	}
}

/*
 * This function polls the NFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 *
 * @param       maxRetries     number of retry attempts (separated by 1 us)
 * @param       useirq         True if IRQ should be used rather than polling
 */
static void wait_op_done(struct imx_nand_host *host, int maxRetries)
{
	while (maxRetries > 0) {
		maxRetries--;
		if (raw_read(NFC_CONFIG2) & NFC_OPS_STAT) {
			raw_write((raw_read(NFC_CONFIG2) & ~NFC_OPS_STAT), NFC_CONFIG2);
			break;
		}
		udelay(1);
	}

	if (maxRetries <= 0)
		printf("%s timeout\n", __func__);
}

/*
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 * @param       useirq  True if IRQ should be used rather than polling
 */
static void send_cmd(struct mtd_info *mtd, u16 cmd, int useirq)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	/* fill command */
	raw_write(cmd, NFC_FLASH_CMD);

	/* send out command */
	raw_write(NFC_CMD, NFC_CONFIG2);

	wait_op_done(host, TROP_US_DELAY);
}

/*
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       useirq  True if IRQ should be used rather than polling
 */
static void send_addr(struct imx_nand_host *host, u16 addr, int useirq)
{
	debug("%s: 0x%04x\n", __func__, addr);

	/* fill address */
	raw_write((addr << NFC_FLASH_ADDR_SHIFT), NFC_FLASH_ADDR);

	/* send out address */
	raw_write(NFC_ADDR, NFC_CONFIG2);

	wait_op_done(host, TROP_US_DELAY);
}

/*
 * This function requests the NFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static u16 get_dev_status(struct imx_nand_host *host)
{
	raw_write(1, NFC_BUF_ADDR);

	/* Read status into main buffer */
	raw_write(NFC_STATUS, NFC_CONFIG2);

	wait_op_done(host, TROP_US_DELAY);

	/* Status is placed in first word of main buffer */
	/* get status, then recovery area 1 data */
	return raw_read(MAIN_AREA1);
}

static void mxc_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	raw_write((raw_read(NFC_CONFIG1) | NFC_ECC_EN), NFC_CONFIG1);
	return;
}

/*
 * Function to record the ECC corrected/uncorrected errors resulted
 * after a page read. This NFC detects and corrects upto to 4 symbols
 * of 9-bits each.
 */
static int mxc_check_ecc_status(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;
	u32 ecc_stat, err;
	int no_subpages = 1;
	int ret = 0;
	u8 ecc_bit_mask, err_limit;

	ecc_bit_mask = (raw_read(NFC_CONFIG1) & NFC_ECC_MODE_4) ? 0x7 : 0xf;
	err_limit = raw_read(NFC_CONFIG1) & NFC_ECC_MODE_4 ? 0x4 : 0x8;

	no_subpages = mtd->writesize >> 9;

	ecc_stat = raw_read(NFC_ECC_STATUS_RESULT);
	do {
		err = ecc_stat & ecc_bit_mask;
		if (err > err_limit) {
			mtd->ecc_stats.failed++;
			printk(KERN_WARNING "UnCorrectable RS-ECC Error\n");
			return -1;
		} else {
			ret += err;
		}
		ecc_stat >>= 4;
	} while (--no_subpages);

	mtd->ecc_stats.corrected += ret;
	pr_debug("%d Symbol Correctable RS-ECC Error\n", ret);

	return ret;
}

/*
 * Function to correct the detected errors. This NFC corrects all the errors
 * detected. So this function just return 0.
 */
static int mxc_nand_correct_data(struct mtd_info *mtd, u_char * dat,
				 u_char * read_ecc, u_char * calc_ecc)
{
	return 0;
}

/*
 * Function to calculate the ECC for the data to be stored in the Nand device.
 * This NFC has a hardware RS(511,503) ECC engine together with the RS ECC
 * CONTROL blocks are responsible for detection  and correction of up to
 * 8 symbols of 9 bits each in 528 byte page.
 * So this function is just return 0.
 */

static int mxc_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
				  u_char *ecc_code)
{
	return 0;
}

/*
 * This function id is used to read the data buffer from the NAND Flash. To
 * read the data from NAND Flash first the data output cycle is initiated by
 * the NFC, which copies the data to RAMbuffer. This data of length \b len is
 * then copied to buffer \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be read from NAND Flash
 * @param       len     number of bytes to be read
 */
static void mxc_nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;
	u16 col = host->col_addr;

	debug("%s: 0x%08x %d\n", __func__, buf, len);

	if (mtd->writesize) {
		int j = mtd->writesize - col;
		int n = mtd->oobsize + j;

		n = min(n, len);

		if (j > 0) {
			if (n > j) {
				memcpy(buf, &data_buf[col], j);
				memcpy(buf + j, &oob_buf[0], n - j);
			} else {
				memcpy(buf, &data_buf[col], n);
			}
		} else {
			col -= mtd->writesize;
			memcpy(buf, &oob_buf[col], len);
		}

		/* update */
		host->col_addr += n;

	} else {
		/* At flash identify phase,
		 * mtd->writesize has not been
		 * set correctly, it should
		 * be zero.And len will less 2
		 */
		memcpy(buf, &data_buf[col], len);

		/* update */
		host->col_addr += len;
	}
}

/*
 * This function reads byte from the NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static uint8_t mxc_nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;
	uint8_t ret;

	/* Check for status request */
	if (host->status_request)
		return get_dev_status(host) & 0xFF;

	mxc_nand_read_buf(mtd, &ret, 1);

	return ret;
}

/*
  * This function reads word from the NAND Flash
  *
  * @param     mtd     MTD structure for the NAND Flash
  *
  * @return    data read from the NAND Flash
  */
static u16 mxc_nand_read_word(struct mtd_info *mtd)
{
	u16 ret;

	mxc_nand_read_buf(mtd, (uint8_t *)&ret, sizeof(u16));

	return ret;
}

/*
 * This function reads byte from the NAND Flash
 *
 * @param     mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static u_char mxc_nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	/* Check for status request */
	if (host->status_request)
		return get_dev_status(host) & 0xFF;

	return mxc_nand_read_word(mtd) & 0xFF;
}

/*
 * This function writes data of length \b len from buffer \b buf to the NAND
 * internal RAM buffer's MAIN area 0.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be written to NAND Flash
 * @param       len     number of bytes to be written
 */
static void mxc_nand_write_buf(struct mtd_info *mtd,
			       const u_char *buf, int len)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;
	u16 col = host->col_addr;
	int j = mtd->writesize - col;
	int n = mtd->oobsize + j;

	n = min(n, len);

	if (j > 0) {
		if (n > j) {
			memcpy(&data_buf[col], buf, j);
			memcpy(&oob_buf[0], buf + j, n - j);
		} else {
			memcpy(&data_buf[col], buf, n);
		}
	} else {
		col -= mtd->writesize;
		memcpy(&oob_buf[col], buf, len);
	}

	/* update */
	host->col_addr += n;
}

/*
 * This function is used by the upper layer to verify the data in NAND Flash
 * with the data in the \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be verified
 * @param       len     length of the data to be verified
 *
 * @return      -EFAULT if error else 0
 *
 */
static int mxc_nand_verify_buf(struct mtd_info *mtd, const u_char *buf,
			       int len)
{
	u_char *s = data_buf;

	const u_char *p = buf;

	for (; len > 0; len--) {
		if (*p++ != *s++)
			return -EFAULT;
	}

	return 0;
}

/*
 * This function is used by upper layer for select and deselect of the NAND
 * chip
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       chip    val indicating select or deselect
 */
static void mxc_nand_select_chip(struct mtd_info *mtd, int chip)
{
}

/*
 * Function to perform the address cycles.
 */
static void mxc_do_addr_cycle(struct mtd_info *mtd, int column, int page_addr)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;
	u32 page_mask = nand_chip->pagemask;

	debug("%s: %d %d\n", __func__, column, page_addr);

	if (column != -1) {
		send_addr(host, column & 0xFF, 0);
		if (mtd->writesize == 2048) {
			/* another col addr cycle for 2k page */
			send_addr(host, (column >> 8) & 0xF, 0);
		} else if (mtd->writesize == 4096) {
			/* another col addr cycle for 4k page */
			send_addr(host, (column >> 8) & 0x1F, 0);
		}
	}
	if (page_addr != -1) {
		do {
			send_addr(host, (page_addr & 0xff), 0);
			page_mask >>= 8;
			page_addr >>= 8;
		} while (page_mask != 0);
	}
}

/*
 * This function is used by the upper layer to write command to NAND Flash for
 * different operations to be carried out on NAND Flash
 *
 * @param       mtd             MTD structure for the NAND Flash
 * @param       command         command for NAND Flash
 * @param       column          column offset for the page read
 * @param       page_addr       page to be read from NAND Flash
 */
static void mxc_nand_command(struct mtd_info *mtd, unsigned command,
			     int column, int page_addr)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	debug("mxc_nand_command (cmd = 0x%x, col = 0x%x, page = 0x%x)\n",
		command, column, page_addr);

	/*
	 * Reset command state information
	 */
	host->status_request = 0;

	/*
	 * Command pre-processing step
	 */
	switch (command) {
	case NAND_CMD_STATUS:
		host->col_addr = 0;
		host->status_request = 1;
		break;

	case NAND_CMD_READ0:
		host->col_addr = column;
		break;

	case NAND_CMD_READOOB:
		host->col_addr = column;
		command = NAND_CMD_READ0;
		break;

	case NAND_CMD_SEQIN:
		if (column) {

			/* FIXME: before send SEQIN command for
			 * partial write,We need read one page out.
			 * FSL NFC does not support partial write
			 * It alway send out 512+ecc+512+ecc ...
			 * for large page nand flash. But for small
			 * page nand flash, it did support SPARE
			 * ONLY operation. But to make driver
			 * simple. We take the same as large page,read
			 * whole page out and update. As for MLC nand
			 * NOP(num of operation) = 1. Partial written
			 * on one programed page is not allowed! We
			 * can't limit it on the driver, it need the
			 * upper layer applicaiton take care it
			 */

			mxc_nand_command(mtd, NAND_CMD_READ0, 0, page_addr);
		}

		host->col_addr = column;
		break;

	case NAND_CMD_PAGEPROG:
		/* FIXME:the NFC interal buffer
		 * access has some limitation, it
		 * does not allow byte access. To
		 * make the code simple and ease use
		 * not every time check the address
		 * alignment.Use the temp buffer
		 * to accomadate the data.since We
		 * know data_buf will be at leat 4
		 * byte alignment, so we can use
		 * memcpy safely
		 */
		memcpy32(MAIN_AREA0, data_buf, mtd->writesize);
		copy_spare(mtd, oob_buf, SPARE_AREA0, mtd->oobsize, 0);

		/* set ram buffer id */
		raw_write(0, NFC_BUF_ADDR);

		/* transfer data from NFC ram to nand */
		raw_write(NFC_INPUT, NFC_CONFIG2);

		wait_op_done(host, TROP_US_DELAY);

		break;

	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
		break;
	}

	send_cmd(mtd, command, 0);

	mxc_do_addr_cycle(mtd, column, page_addr);

	/*
	 * Command post-processing step
	 */
	switch (command) {
	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
		if (mtd->writesize > 512) {
			/* send read confirm command */
			send_cmd(mtd, NAND_CMD_READSTART, 1);
		}

		raw_write(0, NFC_BUF_ADDR);

		/* transfer data from nand to NFC ram */
		raw_write(NFC_OUTPUT, NFC_CONFIG2);

		wait_op_done(host, TROP_US_DELAY);

		/* FIXME, the NFC interal buffer
		 * access has some limitation, it
		 * does not allow byte access. To
		 * make the code simple and ease use
		 * not every time check the address
		 * alignment.Use the temp buffer
		 * to accomadate the data.since We
		 * know data_buf will be at leat 4
		 * byte alignment, so we can use
		 * memcpy safely
		 */
		memcpy32(data_buf, MAIN_AREA0, mtd->writesize);
		copy_spare(mtd, oob_buf, SPARE_AREA0, mtd->oobsize, 1);

		break;

	case NAND_CMD_READID:
		raw_write(0, NFC_BUF_ADDR);

		/* Read ID into main buffer */
		raw_write(NFC_ID, NFC_CONFIG2);

		wait_op_done(host, TROP_US_DELAY);

		host->col_addr = column;
		memcpy32(data_buf, MAIN_AREA0, 2048);

		break;
	}
}

static int mxc_nand_read_oob(struct mtd_info *mtd,
			     struct nand_chip *chip, int page, int sndcmd)
{
	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
		sndcmd = 0;
	}

	memcpy32(chip->oob_poi, oob_buf, mtd->oobsize);

	return sndcmd;
}

static int mxc_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,
			      uint8_t *buf)
{
	debug("%s: 0x%08x\n", __func__, buf);

	mxc_check_ecc_status(mtd);

	memcpy32(buf, data_buf, mtd->writesize);
	memcpy32(chip->oob_poi, oob_buf, mtd->oobsize);

	return 0;
}

static void mxc_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf)
{
	memcpy32(data_buf, buf, mtd->writesize);
	memcpy32(oob_buf, chip->oob_poi, mtd->oobsize);
}

/*
 * We must provide a private bbt decriptor, because the settings from
 * the generic one collide with our ECC hardware.
 */
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern
};

#ifdef CONFIG_ARCH_IMX25

#define RCSR_NFC_FMS		(1 << 8)
#define RCSR_NFC_4K		(1 << 9)
#define RCSR_NFC_16BIT_SEL	(1 << 14)

static void __bare_init mxc_nand_set_writesize(struct mtd_info *mtd, int writesize)
{
	unsigned int rcsr;

	rcsr = readl(IMX_CCM_BASE + CCM_RCSR);
	rcsr &= ~(RCSR_NFC_FMS | RCSR_NFC_4K);

	if (writesize == 2048)
		rcsr |= RCSR_NFC_FMS;
	if (writesize == 4096)
		rcsr |= RCSR_NFC_FMS | RCSR_NFC_4K;

	writel(rcsr, IMX_CCM_BASE + CCM_RCSR);
}

static void __bare_init mxc_nand_set_datawidth(struct mtd_info *mtd, int datawidth)
{
	unsigned int rcsr;

	rcsr = readl(IMX_CCM_BASE + CCM_RCSR);

	if (datawidth == 16)
		rcsr |= RCSR_NFC_16BIT_SEL;
	else
		rcsr &= ~RCSR_NFC_16BIT_SEL;


	writel(rcsr, IMX_CCM_BASE + CCM_RCSR);
}
#endif

static void mxc_nfc_init(struct imx_nand_host *host)
{
	/* Disable interrupt */
	raw_write((raw_read(NFC_CONFIG1) | NFC_INT_MSK), NFC_CONFIG1);

	/* disable spare enable */
	raw_write(raw_read(NFC_CONFIG1) & ~NFC_SP_EN, NFC_CONFIG1);

	/* Unlock the internal RAM Buffer */
	raw_write(NFC_BLS_UNLCOKED, NFC_CONFIG);

	/* Blocks to be unlocked */
	raw_write(0x0, NFC_UNLOCKSTART_BLKADDR);
        raw_write(0xffff, NFC_UNLOCKEND_BLKADDR);

	/* Unlock Block Command for given address range */
	raw_write(NFC_WPC_UNLOCK, NFC_WRPROT);
}

static int __init imxnd_probe(struct device_d *dev)
{
	struct nand_chip *this;
	struct mtd_info *mtd;
	struct imx_nand_platform_data *pdata = dev->platform_data;
	struct imx_nand_host *host;
	u16 tmp;
	int err = 0;

	/* init data buf */
	data_buf = xzalloc(NAND_MAX_PAGESIZE);
	oob_buf = xzalloc(NAND_MAX_OOBSIZE);

	/* Allocate memory for MTD device structure and private data */
	host = kzalloc(sizeof(struct imx_nand_host), GFP_KERNEL);
	if (!host) {
		printk(KERN_ERR "%s: failed to allocate mtd_info\n",
		       __FUNCTION__);
		err = -ENOMEM;
		goto err_out;
	}

	host->dev = dev;
	/* structures must be linked */
	this = &host->nand;
	mtd = &host->mtd;
	mtd->priv = this;

	/* NAND bus width determines access funtions used by upper layer */
	if (pdata->width == 2) {
		this->read_byte = mxc_nand_read_byte16;
		this->options |= NAND_BUSWIDTH_16;
		mxc_nand_set_datawidth(mtd, 16);
	} else
		mxc_nand_set_datawidth(mtd, 8);

	/* 50 us command delay time */
	this->chip_delay = 5;

	this->priv = host;
	this->cmdfunc = mxc_nand_command;
	this->select_chip = mxc_nand_select_chip;
	this->read_byte = mxc_nand_read_byte;
	this->read_word = mxc_nand_read_word;
	this->write_buf = mxc_nand_write_buf;
	this->read_buf = mxc_nand_read_buf;
	this->verify_buf = mxc_nand_verify_buf;

	/* use flash based bbt */
	this->bbt_td = &bbt_main_descr;
	this->bbt_md = &bbt_mirror_descr;

	/* update flash based bbt */
	this->options |= NAND_USE_FLASH_BBT;

	host->regs = (void __iomem *)dev->map_base;

	mxc_nfc_init(host);

	tmp = readw(NFC_CONFIG1);
	tmp |= NFC_INT_MSK;
	writew(tmp, NFC_CONFIG1);

	if (pdata->hw_ecc) {
		this->ecc.read_page = mxc_nand_read_page;
		this->ecc.write_page = mxc_nand_write_page;
		this->ecc.read_oob = mxc_nand_read_oob;
		this->ecc.layout = &nand_hw_eccoob_512;
		this->ecc.calculate = mxc_nand_calculate_ecc;
		this->ecc.hwctl = mxc_nand_enable_hwecc;
		this->ecc.correct = mxc_nand_correct_data;
		this->ecc.mode = NAND_ECC_HW;
		this->ecc.size = 512;
		this->ecc.bytes = 9;
		raw_write(raw_read(NFC_CONFIG1) | NFC_ECC_EN, NFC_CONFIG1);
	} else {
		this->ecc.mode = NAND_ECC_SOFT;
		raw_write(raw_read(NFC_CONFIG1) & ~NFC_ECC_EN, NFC_CONFIG1);
	}

	/* Reset NAND */
	this->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Scan to find existence of the device */
	if (nand_scan_ident(mtd, 1)) {
		err = -ENXIO;
		dev_err(dev, "Unable to find any NAND device\n");
		goto err_out;
	}

	if (mtd->writesize == 2048) {
		mxc_nand_set_writesize(mtd, 2048);
		raw_write(((raw_read(NFC_SPAS) & 0xff00) | NFC_SPAS_64), NFC_SPAS);
		raw_write((raw_read(NFC_CONFIG1) | NFC_ECC_MODE_4), NFC_CONFIG1);
		this->ecc.layout = &nand_hw_eccoob_2k;
	} else if (mtd->writesize == 4096) {
		mxc_nand_set_writesize(mtd, 4096);
		raw_write(((raw_read(NFC_SPAS) & 0xff00) | NFC_SPAS_128), NFC_SPAS);
		raw_write((raw_read(NFC_CONFIG1) | NFC_ECC_MODE_4), NFC_CONFIG1);
		this->ecc.layout = &nand_hw_eccoob_4k;
	} else {
		mxc_nand_set_writesize(mtd, 512);
		this->ecc.layout = &nand_hw_eccoob_512;
	}

	if (nand_scan_tail(mtd)) {
		dev_err(dev, "Unable to find any NAND device.\n");
		err = -ENXIO;
		goto err_out;
	}

	add_mtd_device(mtd);

#ifdef CONFIG_MXC_NAND_LOW_LEVEL_ERASE
	/* Erase all the blocks of a NAND */
	imx_low_erase(mtd);
#endif

	dev->priv = host;

	return 0;
err_out:
	return err;
}

static struct driver_d imx_nand_driver = {
	.name  = "imx_nand",
	.probe = imxnd_probe,
};

/*
 * Main initialization routine
 * @return  0 if successful; non-zero otherwise
 */
static int __init imx_nand_init(void)
{
	return register_driver(&imx_nand_driver);
}

device_initcall(imx_nand_init);

#ifdef CONFIG_NAND_IMX_BOOT

static void __bare_init noinline boot_send_cmd(struct imx_nand_host *host, u16 cmd)
{
	/* fill command */
	raw_write(cmd, NFC_FLASH_CMD);

	/* send out command */
	raw_write(NFC_CMD, NFC_CONFIG2);

	wait_op_done(host, TROP_US_DELAY);
}

static void noinline __bare_init boot_send_addr(struct imx_nand_host *host, u16 addr)
{
	/* fill address */
	raw_write((addr << NFC_FLASH_ADDR_SHIFT), NFC_FLASH_ADDR);

	/* send out address */
	raw_write(NFC_ADDR, NFC_CONFIG2);

	wait_op_done(host, TROP_US_DELAY);
}

static void __bare_init boot_nfc_addr(struct imx_nand_host *host, u32 offs, int pagesize)
{
	switch (pagesize) {
	case 512:
		boot_send_addr(host, offs & 0xff);
		boot_send_addr(host, (offs >> 9) & 0xff);
		boot_send_addr(host, (offs >> 17) & 0xff);
		boot_send_addr(host, (offs >> 25) & 0xff);
		break;
	case 2048:
		boot_send_addr(host, 0);
		boot_send_addr(host, 0);
		boot_send_addr(host, (offs >> 11) & 0xff);
		boot_send_addr(host, (offs >> 19) & 0xff);
		boot_send_addr(host, (offs >> 27) & 0xff);
		break;
	case 4096:
		boot_send_addr(host, 0);
		boot_send_addr(host, 0);
		boot_send_addr(host, (offs >> 12) & 0xff);
		boot_send_addr(host, (offs >> 20) & 0xff);
		boot_send_addr(host, (offs >> 27) & 0xff);
		break;
	}

	if (pagesize > 512)
		boot_send_cmd(host, NAND_CMD_READSTART);
}

static void __bare_init noinline boot_send_read_page(struct imx_nand_host *host, u8 buf_id)
{
	DEBUG(MTD_DEBUG_LEVEL3, "%s(%d)\n", __FUNCTION__, buf_id);

	raw_write(buf_id, NFC_BUF_ADDR);

	/* transfer data from nand to NFC ram */
	raw_write(NFC_OUTPUT, NFC_CONFIG2);

	wait_op_done(host, TROP_US_DELAY);
}

static int __bare_init block_is_bad(struct imx_nand_host *host, u32 offs, int pagesize)
{
	boot_send_cmd(host, NAND_CMD_READ0);
	boot_nfc_addr(host, offs, pagesize);
	boot_send_read_page(host, 0);

	/* FIXME: copied from V1 driver. Is this correct? */
	return (raw_read(SPARE_AREA0) & 0xff) == 0xff ? 0 : 1;
}

void __bare_init imx_nand_load_image(void *dest, int size, int pagesize,
		int blocksize)
{
	struct imx_nand_host _host;
	struct imx_nand_host *host = &_host;
	int width = 1;
	u32 page, block;

	host->regs = (void *)IMX_NAND_BASE;

	debug("%s\n", __func__);

	/* disable spare enable */
	raw_write(raw_read(NFC_CONFIG1) & ~NFC_SP_EN, NFC_CONFIG1);

	/* Unlock the internal RAM Buffer */
	raw_write(NFC_BLS_UNLCOKED, NFC_CONFIG);

	/* Blocks to be unlocked */
	raw_write(0x0, NFC_UNLOCKSTART_BLKADDR);
        raw_write(0xffff, NFC_UNLOCKEND_BLKADDR);

	/* Unlock Block Command for given address range */
	raw_write(NFC_WPC_UNLOCK, NFC_WRPROT);

	if (readl(IMX_CCM_BASE + CCM_RCSR) & (1 << 14))
		width = 2;

	block = page = 0;

	while (1) {
		if (!block_is_bad(host, block * blocksize, pagesize)) {
			page = 0;
			while (page * pagesize < blocksize) {
				debug("page: %d block: %d dest: %p src "
						"0x%08x\n",
						page, block, dest,
						block * blocksize +
						page * pagesize);

				boot_send_cmd(host, NAND_CMD_READ0);
				boot_nfc_addr(host, block * blocksize +
						page * pagesize, pagesize);
				boot_send_read_page(host, 0);
				page++;
				memcpy32(dest, MAIN_AREA0, pagesize);
				dest += pagesize;
				size -= pagesize;
				if (size <= 0)
					return;
			}
		}
		block++;
	}
}

#ifdef IMX_NAND_BOOT_DEBUG
#include <command.h>
static int do_nand_boot_test(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	void *dest;
	int size, pagesize, blocksize;

	if (argc < 4) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	dest = (void *)strtoul_suffix(argv[1], NULL, 0);
	size = strtoul_suffix(argv[2], NULL, 0);
	pagesize = strtoul_suffix(argv[3], NULL, 0);
	blocksize = strtoul_suffix(argv[4], NULL, 0);

	imx_nand_load_image(dest, size, pagesize, blocksize * 1024);

	return 0;
}

static const __maybe_unused char cmd_nand_boot_test_help[] =
"Usage: nand_boot_test <dest> <size> <pagesize> <blocksize in kiB>\n";

U_BOOT_CMD_START(nand_boot_test)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_nand_boot_test,
	.usage		= "load an image from NAND",
	U_BOOT_CMD_HELP(cmd_nand_boot_test_help)
U_BOOT_CMD_END

#endif
#endif
