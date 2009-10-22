/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2008 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * MX21 Hardware contains a bug which causes HW ECC to fail for two
 * consecutive read pages containing 1bit Errors (See MX21 Chip Erata,
 * Erratum 16). Use software ECC for this chip.
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

#define NFMS (*((volatile u32 *)(IMX_SYSTEM_CTL_BASE + 0x14)))
#define NFMS_BIT 5

#define DVR_VER "2.0"

/*
 * Addresses for NFC registers
 */
#define NFC_BUF_SIZE		0xE00
#define NFC_BUF_ADDR		0xE04
#define NFC_FLASH_ADDR		0xE06
#define NFC_FLASH_CMD		0xE08
#define NFC_CONFIG		0xE0A
#define NFC_ECC_STATUS_RESULT	0xE0C
#define NFC_RSLTMAIN_AREA	0xE0E
#define NFC_RSLTSPARE_AREA	0xE10
#define NFC_WRPROT		0xE12
#define NFC_UNLOCKSTART_BLKADDR	0xE14
#define NFC_UNLOCKEND_BLKADDR	0xE16
#define NFC_NF_WRPRST		0xE18
#define NFC_CONFIG1		0xE1A
#define NFC_CONFIG2		0xE1C

/*
 * Addresses for NFC RAM BUFFER Main area 0
 */
#define MAIN_AREA0		0x000
#define MAIN_AREA1		0x200
#define MAIN_AREA2		0x400
#define MAIN_AREA3		0x600

/*
 * Addresses for NFC SPARE BUFFER Spare area 0
 */
#define SPARE_AREA0		0x800
#define SPARE_AREA1		0x810
#define SPARE_AREA2		0x820
#define SPARE_AREA3		0x830

/*
 * Set INT to 0, FCMD to 1, rest to 0 in NFC_CONFIG2 Register for Command
 * operation
 */
#define NFC_CMD            0x1

/*
 * Set INT to 0, FADD to 1, rest to 0 in NFC_CONFIG2 Register for Address
 * operation
 */
#define NFC_ADDR           0x2

/*
 * Set INT to 0, FDI to 1, rest to 0 in NFC_CONFIG2 Register for Input
 * operation
 */
#define NFC_INPUT          0x4

/*
 * Set INT to 0, FDO to 001, rest to 0 in NFC_CONFIG2 Register for Data Output
 * operation
 */
#define NFC_OUTPUT         0x8

/*
 * Set INT to 0, FD0 to 010, rest to 0 in NFC_CONFIG2 Register for Read ID
 * operation
 */
#define NFC_ID             0x10

/*
 * Set INT to 0, FDO to 100, rest to 0 in NFC_CONFIG2 Register for Read Status
 * operation
 */
#define NFC_STATUS         0x20

/*
 * Set INT to 1, rest to 0 in NFC_CONFIG2 Register for Read Status
 * operation
 */
#define NFC_INT            0x8000

#define NFC_SP_EN           (1 << 2)
#define NFC_ECC_EN          (1 << 3)
#define NFC_INT_MSK         (1 << 4)
#define NFC_BIG             (1 << 5)
#define NFC_RST             (1 << 6)
#define NFC_CE              (1 << 7)
#define NFC_ONE_CYCLE       (1 << 8)

#ifdef CONFIG_NAND_IMX_BOOT
#define __nand_boot_init __bare_init
#else
#define __nand_boot_init
#endif

struct imx_nand_host {
	struct mtd_info		mtd;		
	struct nand_chip	nand;
	struct mtd_partition	*parts;
	struct device_d		*dev;

	void __iomem		*regs;
	int			spare_only;
	int			status_request;
	u16			col_addr;
	struct clk		*clk;

	int			pagesize_2k;
};

/*
 * Macros to get byte and bit positions of ECC
 */
#define COLPOS(x)  ((x) >> 3)
#define BITPOS(x) ((x)& 0xf)

/* Define single bit Error positions in Main & Spare area */
#define MAIN_SINGLEBIT_ERROR 0x4
#define SPARE_SINGLEBIT_ERROR 0x1

/*
 * OOB placement block for use with hardware ecc generation
 */
static struct nand_ecclayout nand_hw_eccoob_8 = {
	.eccbytes = 5,
	.eccpos = {6, 7, 8, 9, 10},
	.oobfree = {{0, 5}, {11, 5}}
};

static struct nand_ecclayout nand_hw_eccoob_16 = {
	.eccbytes = 5,
	.eccpos = {6, 7, 8, 9, 10},
	.oobfree = {{0, 6}, {12, 4}}
};

/*
 * This function polls the NANDFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 *
 * @param       max_retries     number of retry attempts (separated by 1 us)
 * @param       param           parameter for debug
 */
static void __nand_boot_init wait_op_done(struct imx_nand_host *host, u16 param)
{
	u32 tmp;
	int i;

	/* This is a timeout of roughly 15ms on my system. We
	 * need about 2us, but be generous. Don't use udelay
	 * here as we might be here from nand booting.
	 */
	for (i = 0; i < 100000; i++) {
		if (readw(host->regs + NFC_CONFIG2) & NFC_INT) {
			tmp = readw(host->regs + NFC_CONFIG2);
			tmp  &= ~NFC_INT;
			writew(tmp, host->regs + NFC_CONFIG2);
			return;
		}
	}
}

/*
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 */
static void __nand_boot_init send_cmd(struct imx_nand_host *host, u16 cmd)
{
	MTD_DEBUG(MTD_DEBUG_LEVEL3, "send_cmd(host, 0x%x)\n", cmd);

	writew(cmd, host->regs + NFC_FLASH_CMD);
	writew(NFC_CMD, host->regs + NFC_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host, cmd);
}

/*
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       islast  True if this is the last address cycle for command
 */
static void __nand_boot_init send_addr(struct imx_nand_host *host, u16 addr)
{
	MTD_DEBUG(MTD_DEBUG_LEVEL3, "send_addr(host, 0x%x %d)\n", addr, islast);

	writew(addr, host->regs + NFC_FLASH_ADDR);
	writew(NFC_ADDR, host->regs + NFC_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host, addr);
}

/*
 * This function requests the NANDFC to initate the transfer
 * of data currently in the NANDFC RAM buffer to the NAND device.
 *
 * @param	buf_id	      Specify Internal RAM Buffer number (0-3)
 * @param       spare_only    set true if only the spare area is transferred
 */
static void send_prog_page(struct imx_nand_host *host, u8 buf_id,
		int spare_only)
{
	MTD_DEBUG(MTD_DEBUG_LEVEL3, "send_prog_page (%d)\n", spare_only);

	/* NANDFC buffer 0 is used for page read/write */

	writew(buf_id, host->regs + NFC_BUF_ADDR);

	/* Configure spare or page+spare access */
	if (!host->pagesize_2k) {
		u16 config1 = readw(host->regs + NFC_CONFIG1);
		if (spare_only)
			config1 |= NFC_SP_EN;
		else
			config1 &= ~(NFC_SP_EN);
		writew(config1, host->regs + NFC_CONFIG1);
	}

	writew(NFC_INPUT, host->regs + NFC_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host, spare_only);
}

/*
 * This function requests the NANDFC to initate the transfer
 * of data from the NAND device into in the NANDFC ram buffer.
 *
 * @param  	buf_id		Specify Internal RAM Buffer number (0-3)
 * @param       spare_only    	set true if only the spare area is transferred
 */
static void __nand_boot_init send_read_page(struct imx_nand_host *host,
		u8 buf_id, int spare_only)
{
	MTD_DEBUG(MTD_DEBUG_LEVEL3, "send_read_page (%d)\n", spare_only);

	/* NANDFC buffer 0 is used for page read/write */
	writew(buf_id, host->regs + NFC_BUF_ADDR);

	/* Configure spare or page+spare access */
	if (!host->pagesize_2k) {
		u32 config1 = readw(host->regs + NFC_CONFIG1);
		if (spare_only)
			config1 |= NFC_SP_EN;
		else
			config1 &= ~NFC_SP_EN;
		writew(config1, host->regs + NFC_CONFIG1);
	}

	writew(NFC_OUTPUT, host->regs + NFC_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host, spare_only);
}

/*
 * This function requests the NANDFC to perform a read of the
 * NAND device ID.
 */
static void send_read_id(struct imx_nand_host *host)
{
	struct nand_chip *this = &host->nand;
	u16 tmp;

	/* NANDFC buffer 0 is used for device ID output */
	writew(0x0, host->regs + NFC_BUF_ADDR);

	/* Read ID into main buffer */
	tmp = readw(host->regs + NFC_CONFIG1);
	tmp &= ~NFC_SP_EN;
	writew(tmp, host->regs + NFC_CONFIG1);

	writew(NFC_ID, host->regs + NFC_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host, 0);

	if (this->options & NAND_BUSWIDTH_16) {
		volatile u16 *mainbuf = host->regs + MAIN_AREA0;

		/*
		 * Pack the every-other-byte result for 16-bit ID reads
		 * into every-byte as the generic code expects and various
		 * chips implement.
		 */

		mainbuf[0] = (mainbuf[0] & 0xff) | ((mainbuf[1] & 0xff) << 8);
		mainbuf[1] = (mainbuf[2] & 0xff) | ((mainbuf[3] & 0xff) << 8);
		mainbuf[2] = (mainbuf[4] & 0xff) | ((mainbuf[5] & 0xff) << 8);
	}
}

/*
 * This function requests the NANDFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static u16 get_dev_status(struct imx_nand_host *host)
{
	volatile u16 *mainbuf = host->regs + MAIN_AREA1;
	u32 store;
	u16 ret, tmp;
	/* Issue status request to NAND device */

	/* store the main area1 first word, later do recovery */
	store = *((u32 *) mainbuf);
	/*
	 * NANDFC buffer 1 is used for device status to prevent
	 * corruption of read/write buffer on status requests.
	 */
	writew(1, host->regs + NFC_BUF_ADDR);

	/* Read status into main buffer */
	tmp = readw(host->regs + NFC_CONFIG1);
	tmp &= ~NFC_SP_EN;
	writew(tmp, host->regs + NFC_CONFIG1);

	writew(NFC_STATUS, host->regs + NFC_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host, 0);

	/* Status is placed in first word of main buffer */
	/* get status, then recovery area 1 data */
	ret = mainbuf[0];
	*((u32 *) mainbuf) = store;

	return ret;
}

/*
 * This function is used by upper layer to checks if device is ready
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return  0 if device is busy else 1
 */
static int imx_nand_dev_ready(struct mtd_info *mtd)
{
	/*
	 * NFC handles R/B internally.Therefore,this function
	 * always returns status as ready.
	 */
	return 1;
}

static void imx_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	/*
	 * If HW ECC is enabled, we turn it on during init.  There is
	 * no need to enable again here.
	 */
}

static int imx_nand_correct_data(struct mtd_info *mtd, u_char * dat,
				 u_char * read_ecc, u_char * calc_ecc)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	/*
	 * 1-Bit errors are automatically corrected in HW.  No need for
	 * additional correction.  2-Bit errors cannot be corrected by
	 * HW ECC, so we need to return failure
	 */
	u16 ecc_status = readw(host->regs + NFC_ECC_STATUS_RESULT);

	if (((ecc_status & 0x3) == 2) || ((ecc_status >> 2) == 2)) {
		MTD_DEBUG(MTD_DEBUG_LEVEL0,
		      "MXC_NAND: HWECC uncorrectable 2-bit ECC error\n");
		return -1;
	}

	return 0;
}

static int imx_nand_calculate_ecc(struct mtd_info *mtd, const u_char * dat,
				  u_char * ecc_code)
{
	return 0;
}

/*
 * This function reads byte from the NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 *
 * @return    data read from the NAND Flash
 */
static u_char imx_nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	u_char ret = 0;
	u16 col, rdword;
	volatile u16 *mainbuf = host->regs + MAIN_AREA0;
	volatile u16 *sparebuf = host->regs + SPARE_AREA0;

	/* Check for status request */
	if (host->status_request)
		return get_dev_status(host) & 0xFF;

	/* Get column for 16-bit access */
	col = host->col_addr >> 1;

	/* If we are accessing the spare region */
	if (host->spare_only)
		rdword = sparebuf[col];
	else
		rdword = mainbuf[col];

	/* Pick upper/lower byte of word from RAM buffer */
	if (host->col_addr & 0x1)
		ret = (rdword >> 8) & 0xFF;
	else
		ret = rdword & 0xFF;

	/* Update saved column address */
	host->col_addr++;

	return ret;
}

/*
  * This function reads word from the NAND Flash
  *
  * @param       mtd     MTD structure for the NAND Flash
  *
  * @return    data read from the NAND Flash
  */
static u16 imx_nand_read_word(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	u16 col;
	u16 rdword, ret;
	volatile u16 *p;

	MTD_DEBUG(MTD_DEBUG_LEVEL3,
	      "imx_nand_read_word(col = %d)\n", host->col_addr);

	col = host->col_addr;
	/* Adjust saved column address */
	if (col < mtd->writesize && host->spare_only)
		col += mtd->writesize;

	if (col < mtd->writesize)
		p = (host->regs + MAIN_AREA0) + (col >> 1);
	else
		p = (host->regs + SPARE_AREA0) + ((col - mtd->writesize) >> 1);

	if (col & 1) {
		rdword = *p;
		ret = (rdword >> 8) & 0xff;
		rdword = *(p + 1);
		ret |= (rdword << 8) & 0xff00;
	} else
		ret = *p;

	/* Update saved column address */
	host->col_addr = col + 2;

	return ret;
}

static void __nand_boot_init memcpy32(void *trg, const void *src, int size)
{
	int i;
	unsigned int *t = trg;
	unsigned const int *s = src;

	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

/*
 * This function writes data of length \b len to buffer \b buf. The data to be
 * written on NAND Flash is first copied to RAMbuffer. After the Data Input
 * Operation by the NFC, the data is written to NAND Flash
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be written to NAND Flash
 * @param       len     number of bytes to be written
 */
static void imx_nand_write_buf(struct mtd_info *mtd,
			       const u_char *buf, int len)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	int n;
	int col;
	int i = 0;

	MTD_DEBUG(MTD_DEBUG_LEVEL3,
	      "imx_nand_write_buf(col = %d, len = %d)\n", host->col_addr,
	      len);

	col = host->col_addr;

	/* Adjust saved column address */
	if (col < mtd->writesize && host->spare_only)
		col += mtd->writesize;

	n = mtd->writesize + mtd->oobsize - col;
	n = min(len, n);

	MTD_DEBUG(MTD_DEBUG_LEVEL3,
	      "%s:%d: col = %d, n = %d\n", __FUNCTION__, __LINE__, col, n);

	while (n) {
		volatile u32 *p;
		if (col < mtd->writesize)
			p = (volatile u32 *)((ulong) (host->regs + MAIN_AREA0)
					+ (col & ~3));
		else
			p = (volatile u32 *)((ulong) (host->regs + SPARE_AREA0)
					- mtd->writesize + (col & ~3));

		MTD_DEBUG(MTD_DEBUG_LEVEL3, "%s:%d: p = %p\n", __FUNCTION__,
		      __LINE__, p);

		if (((col | (int)&buf[i]) & 3) || n < 16) {
			u32 data = 0;

			if (col & 3 || n < 4)
				data = *p;

			switch (col & 3) {
			case 0:
				if (n) {
					data = (data & 0xffffff00) |
					    (buf[i++] << 0);
					n--;
					col++;
				}
			case 1:
				if (n) {
					data = (data & 0xffff00ff) |
					    (buf[i++] << 8);
					n--;
					col++;
				}
			case 2:
				if (n) {
					data = (data & 0xff00ffff) |
					    (buf[i++] << 16);
					n--;
					col++;
				}
			case 3:
				if (n) {
					data = (data & 0x00ffffff) |
					    (buf[i++] << 24);
					n--;
					col++;
				}
			}

			*p = data;
		} else {
			int m = mtd->writesize - col;

			if (col >= mtd->writesize)
				m += mtd->oobsize;

			m = min(n, m) & ~3;

			MTD_DEBUG(MTD_DEBUG_LEVEL3,
			      "%s:%d: n = %d, m = %d, i = %d, col = %d\n",
			      __FUNCTION__, __LINE__, n, m, i, col);

#ifdef CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS
			memcpy((void *)(p), &buf[i], m);
#else
			memcpy32((void *)(p), &buf[i], m);
#endif
			col += m;
			i += m;
			n -= m;
		}
	}
	/* Update saved column address */
	host->col_addr = col;
}

/*
 * This function is used to read the data buffer from the NAND Flash. To
 * read the data from NAND Flash first the data output cycle is initiated by
 * the NFC, which copies the data to RAMbuffer. This data of length \b len is
 * then copied to buffer \b buf.
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       buf     data to be read from NAND Flash
 * @param       len     number of bytes to be read
 */
static void imx_nand_read_buf(struct mtd_info *mtd, u_char * buf, int len)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	int n;
	int col;
	int i = 0;

	MTD_DEBUG(MTD_DEBUG_LEVEL3,
	      "imx_nand_read_buf(col = %d, len = %d)\n", host->col_addr,
	      len);

	col = host->col_addr;
	/* Adjust saved column address */
	if (col < mtd->writesize && host->spare_only)
		col += mtd->writesize;

	n = mtd->writesize + mtd->oobsize - col;
	n = min(len, n);

	while (n) {
		volatile u32 *p;

		if (col < mtd->writesize)
			p = (volatile u32 *)((ulong) (host->regs + MAIN_AREA0)
					+ (col & ~3));
		else
			p = (volatile u32 *)((ulong) (host->regs + SPARE_AREA0)
					- mtd->writesize + (col & ~3));
		if (((col | (int)&buf[i]) & 3) || n < 16) {
			u32 data;
			data = *p;
			switch (col & 3) {
			case 0:
				if (n) {
					buf[i++] = (u8) (data);
					n--;
					col++;
				}
			case 1:
				if (n) {
					buf[i++] = (u8) (data >> 8);
					n--;
					col++;
				}
			case 2:
				if (n) {
					buf[i++] = (u8) (data >> 16);
					n--;
					col++;
				}
			case 3:
				if (n) {
					buf[i++] = (u8) (data >> 24);
					n--;
					col++;
				}
			}
		} else {
			int m = mtd->writesize - col;
			if (col >= mtd->writesize)
				m += mtd->oobsize;

			m = min(n, m) & ~3;
#ifdef CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS
			memcpy(&buf[i], (void *)p, m);
#else
			memcpy32(&buf[i], (void *)(p), m);
#endif
			col += m;
			i += m;
			n -= m;
		}
	}
	/* Update saved column address */
	host->col_addr = col;

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
static int
imx_nand_verify_buf(struct mtd_info *mtd, const u_char * buf, int len)
{
	return -EFAULT;
}

/*
 * This function is used by upper layer for select and deselect of the NAND
 * chip
 *
 * @param       mtd     MTD structure for the NAND Flash
 * @param       chip    val indicating select or deselect
 */
static void imx_nand_select_chip(struct mtd_info *mtd, int chip)
{
#ifdef CONFIG_MTD_NAND_MXC_FORCE_CE
	u16 tmp;

	if (chip > 0) {
		MTD_DEBUG(MTD_DEBUG_LEVEL0,
		      "ERROR:  Illegal chip select (chip = %d)\n", chip);
		return;
	}

	if (chip == -1) {
		tmp = readw(host->regs + NFC_CONFIG1);
		tmp &= ~NFC_CE;
		writew(tmp, host->regs + NFC_CONFIG1);
		return;
	}

	tmp = readw(host->regs + NFC_CONFIG1);
	tmp |= NFC_CE;
	writew(tmp, host->regs + NFC_CONFIG1);
#endif
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
static void imx_nand_command(struct mtd_info *mtd, unsigned command,
			     int column, int page_addr)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	MTD_DEBUG(MTD_DEBUG_LEVEL3,
	      "imx_nand_command (cmd = 0x%x, col = 0x%x, page = 0x%x)\n",
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
		host->spare_only = 0;
		break;

	case NAND_CMD_READOOB:
		host->col_addr = column;
		host->spare_only = 1;
		if (host->pagesize_2k)
			/* only READ0 is valid */
			command = NAND_CMD_READ0;
		break;

	case NAND_CMD_SEQIN:
		if (column >= mtd->writesize) {
			if (host->pagesize_2k) {
				/**
				  * FIXME: before send SEQIN command for write
				  * OOB, we must read one page out. For K9F1GXX
				  * has no READ1 command to set current HW
				  * pointer to spare area, we must write the
				  * whole page including OOB together.
				  */
				/* call ourself to read a page */
				imx_nand_command(mtd, NAND_CMD_READ0, 0,
						 page_addr);
			}
			host->col_addr = column - mtd->writesize;
			host->spare_only = 1;
			/* Set program pointer to spare region */
			if (!host->pagesize_2k)
				send_cmd(host, NAND_CMD_READOOB);
		} else {
			host->spare_only = 0;
			host->col_addr = column;
			/* Set program pointer to page start */
			if (!host->pagesize_2k)
				send_cmd(host, NAND_CMD_READ0);
		}
		break;

	case NAND_CMD_PAGEPROG:
		send_prog_page(host, 0, host->spare_only);

		if (host->pagesize_2k) {
			/* data in 4 areas datas */
			send_prog_page(host, 1, host->spare_only);
			send_prog_page(host, 2, host->spare_only);
			send_prog_page(host, 3, host->spare_only);
		}

		break;

	case NAND_CMD_ERASE1:
		break;
	}

	/*
	 * Write out the command to the device.
	 */
	send_cmd(host, command);

	/*
	 * Write out column address, if necessary
	 */
	if (column != -1) {
		/*
		 * MXC NANDFC can only perform full page+spare or
		 * spare-only read/write.  When the upper layers
		 * layers perform a read/write buf operation,
		 * we will used the saved column adress to index into
		 * the full page.
		 */
		send_addr(host, 0);
		if (host->pagesize_2k)
			/* another col addr cycle for 2k page */
			send_addr(host, 0);
	}

	/*
	 * Write out page address, if necessary
	 */
	if (page_addr != -1) {
		send_addr(host, (page_addr & 0xff));	/* paddr_0 - p_addr_7 */

		if (host->pagesize_2k) {
			send_addr(host, (page_addr >> 8) & 0xFF);
			if (mtd->size >= 0x10000000) {
				send_addr(host, (page_addr >> 16) & 0xff);
			}
		} else {
			/* One more address cycle for higher density devices */
			if (mtd->size >= 0x4000000) {
				/* paddr_8 - paddr_15 */
				send_addr(host, (page_addr >> 8) & 0xff);
				send_addr(host, (page_addr >> 16) & 0xff);
			} else
				/* paddr_8 - paddr_15 */
				send_addr(host, (page_addr >> 8) & 0xff);
		}
	}

	/*
	 * Command post-processing step
	 */
	switch (command) {

	case NAND_CMD_RESET:
		break;

	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
		if (host->pagesize_2k) {
			/* send read confirm command */
			send_cmd(host, NAND_CMD_READSTART);
			/* read for each AREA */
			send_read_page(host, 0, host->spare_only);
			send_read_page(host, 1, host->spare_only);
			send_read_page(host, 2, host->spare_only);
			send_read_page(host, 3, host->spare_only);
		} else {
			send_read_page(host, 0, host->spare_only);
		}
		break;

	case NAND_CMD_READID:
		host->col_addr = 0;
		send_read_id(host);
		break;

	case NAND_CMD_PAGEPROG:
		break;

	case NAND_CMD_STATUS:
		break;

	case NAND_CMD_ERASE2:
		break;
	}
}

#ifdef CONFIG_MXC_NAND_LOW_LEVEL_ERASE
static void imx_low_erase(struct mtd_info *mtd)
{

	struct nand_chip *this = mtd->priv;
	unsigned int page_addr, addr;
	u_char status;

	MTD_DEBUG(MTD_DEBUG_LEVEL0, "MXC_ND : imx_low_erase:Erasing NAND\n");
	for (addr = 0; addr < this->chipsize; addr += mtd->erasesize) {
		page_addr = addr / mtd->writesize;
		imx_nand_command(mtd, NAND_CMD_ERASE1, -1, page_addr);
		imx_nand_command(mtd, NAND_CMD_ERASE2, -1, -1);
		imx_nand_command(mtd, NAND_CMD_STATUS, -1, -1);
		status = imx_nand_read_byte(mtd);
		if (status & NAND_STATUS_FAIL) {
			printk(KERN_ERR
			       "ERASE FAILED(block = %d,status = 0x%x)\n",
			       addr / mtd->erasesize, status);
		}
	}

}
#endif
/*
 * This function is called during the driver binding process.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and
 *                remove functions
 *
 * @return  The function always returns 0.
 */

static int __init imxnd_probe(struct device_d *dev)
{
	struct nand_chip *this;
	struct mtd_info *mtd;
	struct imx_nand_platform_data *pdata = dev->platform_data;
	struct imx_nand_host *host;
	u16 tmp;
	int err = 0;
#ifdef CONFIG_ARCH_IMX27
	PCCR1 |= PCCR1_NFC_BAUDEN;
#endif
	/* Allocate memory for MTD device structure and private data */
	host = kzalloc(sizeof(struct imx_nand_host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	host->dev = dev;
	/* structures must be linked */
	this = &host->nand;
	mtd = &host->mtd;
	mtd->priv = this;

	/* 50 us command delay time */
	this->chip_delay = 5;

	this->priv = host;
	this->dev_ready = imx_nand_dev_ready;
	this->cmdfunc = imx_nand_command;
	this->select_chip = imx_nand_select_chip;
	this->read_byte = imx_nand_read_byte;
	this->read_word = imx_nand_read_word;
	this->write_buf = imx_nand_write_buf;
	this->read_buf = imx_nand_read_buf;
	this->verify_buf = imx_nand_verify_buf;
#if 0
	host->clk = clk_get(&pdev->dev, "nfc_clk");
	if (IS_ERR(host->clk))
		goto eclk;

	clk_enable(host->clk);
#endif

	host->regs = (void __iomem *)dev->map_base;

	tmp = readw(host->regs + NFC_CONFIG1);
	tmp |= NFC_INT_MSK;
	writew(tmp, host->regs + NFC_CONFIG1);

	if (pdata->hw_ecc) {
		this->ecc.calculate = imx_nand_calculate_ecc;
		this->ecc.hwctl = imx_nand_enable_hwecc;
		this->ecc.correct = imx_nand_correct_data;
		this->ecc.mode = NAND_ECC_HW;
		this->ecc.size = 512;
		this->ecc.bytes = 3;
		this->ecc.layout = &nand_hw_eccoob_8;
		tmp = readw(host->regs + NFC_CONFIG1);
		tmp |= NFC_ECC_EN;
		writew(tmp, host->regs + NFC_CONFIG1);
	} else {
		this->ecc.size = 512;
		this->ecc.bytes = 3;
		this->ecc.layout = &nand_hw_eccoob_8;
		this->ecc.mode = NAND_ECC_SOFT;
		tmp = readw(host->regs + NFC_CONFIG1);
		tmp &= ~NFC_ECC_EN;
		writew(tmp, host->regs + NFC_CONFIG1);
	}

	/* Reset NAND */
	this->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	writew(0x2, host->regs + NFC_CONFIG);

	/* Blocks to be unlocked */
	writew(0x0, host->regs + NFC_UNLOCKSTART_BLKADDR);
	writew(0x4000, host->regs + NFC_UNLOCKEND_BLKADDR);

	/* Unlock Block Command for given address range */
	writew(0x4, host->regs + NFC_WRPROT);

	/* NAND bus width determines access funtions used by upper layer */
	if (pdata->width == 2) {
		this->options |= NAND_BUSWIDTH_16;
		this->ecc.layout = &nand_hw_eccoob_16;
	}

	host->pagesize_2k = 0;

	this->options |= NAND_SKIP_BBTSCAN;

	/* Scan to find existence of the device */
	if (nand_scan(mtd, 1)) {
		MTD_DEBUG(MTD_DEBUG_LEVEL0,
		      "MXC_ND: Unable to find any NAND device.\n");
		err = -ENXIO;
		goto escan;
	}

	add_mtd_device(mtd);

#ifdef CONFIG_MXC_NAND_LOW_LEVEL_ERASE
	/* Erase all the blocks of a NAND */
	imx_low_erase(mtd);
#endif

	dev->priv = host;

	return 0;

escan:
	kfree(host);

	return err;

}

static struct driver_d imx_nand_driver = {
	.name  = "imx_nand",
	.probe = imxnd_probe,
};

#ifdef CONFIG_NAND_IMX_BOOT

static void __nand_boot_init nfc_addr(struct imx_nand_host *host, u32 offs)
{
	send_addr(host, offs & 0xff);
	send_addr(host, (offs >> 9) & 0xff);
	send_addr(host, (offs >> 17) & 0xff);
	send_addr(host, (offs >> 25) & 0xff);
}

static int __nand_boot_init block_is_bad(struct imx_nand_host *host, u32 offs)
{
	send_cmd(host, NAND_CMD_READOOB);
	nfc_addr(host, offs);
	send_read_page(host, 0, 1);

	return (readw(host->regs + SPARE_AREA0 + 4) & 0xff) == 0xff ? 0 : 1;
}

void __nand_boot_init imx_nand_load_image(void *dest, int size, int pagesize,
		int blocksize)
{
	struct imx_nand_host host;
	u32 tmp, page, block;

	host.regs = (void __iomem *)IMX_NFC_BASE;
	host.pagesize_2k = (pagesize == 2048);

	send_cmd(&host, NAND_CMD_RESET);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	writew(0x2, host.regs + NFC_CONFIG);

	/* Blocks to be unlocked */
	writew(0x0, host.regs + NFC_UNLOCKSTART_BLKADDR);
	writew(0x4000, host.regs + NFC_UNLOCKEND_BLKADDR);

	/* Unlock Block Command for given address range */
	writew(0x4, host.regs + NFC_WRPROT);

	tmp = readw(host.regs + NFC_CONFIG1);
	tmp |= NFC_ECC_EN;
	writew(tmp, host.regs + NFC_CONFIG1);

	block = page = 0;

	while (1) {
		if (!block_is_bad(&host, block * blocksize)) {
			page = 0;
			while (page * pagesize < blocksize) {
				debug("page: %d block: %d dest: %p src "
						"0x%08x\n",
						page, block, dest,
						block * blocksize +
						page * pagesize);

				send_cmd(&host, NAND_CMD_READ0);
				nfc_addr(&host, block * blocksize +
						page * pagesize);
				send_read_page(&host, 0, 0);
				page++;
				memcpy32(dest, host.regs, 512);
				dest += pagesize;
				size -= pagesize;
				if (size <= 0)
					return;
			}
		} else
			debug("skip bad block at 0x%08x\n", block * blocksize);
		block++;
	}
}
#define CONFIG_NAND_IMX_BOOT_DEBUG
#ifdef CONFIG_NAND_IMX_BOOT_DEBUG
#include <command.h>

static int do_nand_boot_test(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	void *dest;
	int size, pagesize, blocksize;

	if (argc < 4)
		return COMMAND_ERROR_USAGE;

	dest = (void *)strtoul_suffix(argv[1], NULL, 0);
	size = strtoul_suffix(argv[2], NULL, 0);
	pagesize = strtoul_suffix(argv[3], NULL, 0);
	blocksize = strtoul_suffix(argv[4], NULL, 0);

	imx_nand_load_image(dest, size, pagesize, blocksize);

	return 0;
}

static const __maybe_unused char cmd_nand_boot_test_help[] =
"Usage: nand_boot_test <dest> <size> <pagesize> <blocksize>\n";

U_BOOT_CMD_START(nand_boot_test)
	.cmd		= do_nand_boot_test,
	.usage		= "list a file or directory",
	U_BOOT_CMD_HELP(cmd_nand_boot_test_help)
U_BOOT_CMD_END
#endif

#endif /* CONFIG_NAND_IMX_BOOT */

/*
 * Main initialization routine
 * @return  0 if successful; non-zero otherwise
 */
static int __init imx_nand_init(void)
{
	return register_driver(&imx_nand_driver);
}

device_initcall(imx_nand_init);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC NAND MTD driver");
MODULE_LICENSE("GPL");
