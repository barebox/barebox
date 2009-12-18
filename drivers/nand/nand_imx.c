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
#include <mach/generic.h>
#include <mach/imx-nand.h>
#include <mach/imx-regs.h>
#include <asm/io.h>
#include <errno.h>

#define DVR_VER "2.0"

#define nfc_is_v21()		(cpu_is_mx25() || cpu_is_mx35() || cpu_is_mx21())
#define nfc_is_v1()		(cpu_is_mx31() || cpu_is_mx27())

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
#define NFC_SPAS		0xe10
#define NFC_WRPROT		0xE12
#define NFC_V1_UNLOCKSTART_BLKADDR	0xe14
#define NFC_V1_UNLOCKEND_BLKADDR	0xe16
#define NFC_V21_UNLOCKSTART_BLKADDR	0xe20
#define NFC_V21_UNLOCKEND_BLKADDR	0xe22
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

#define NFC_ECC_MODE        (1 << 0)
#define NFC_SP_EN           (1 << 2)
#define NFC_ECC_EN          (1 << 3)
#define NFC_INT_MSK         (1 << 4)
#define NFC_BIG             (1 << 5)
#define NFC_RST             (1 << 6)
#define NFC_CE              (1 << 7)
#define NFC_ONE_CYCLE       (1 << 8)

#define NFC_SPAS_16			 8
#define NFC_SPAS_64			 32
#define NFC_SPAS_128			 64
#define NFC_SPAS_218			 109

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

	void			*spare0;
	void			*main_area0;
	void			*main_area1;

	void __iomem		*base;
	void __iomem		*regs;
	int			status_request;
	struct clk		*clk;

	int			pagesize_2k;
	uint8_t			*data_buf;
	unsigned int		buf_start;
	int			spare_len;

};

/*
 * OOB placement block for use with hardware ecc generation
 */
static struct nand_ecclayout nandv1_hw_eccoob_smallpage = {
	.eccbytes = 5,
	.eccpos = {6, 7, 8, 9, 10},
	.oobfree = {{0, 5}, {12, 4}}
};

static struct nand_ecclayout nandv1_hw_eccoob_largepage = {
	.eccbytes = 20,
	.eccpos = {6, 7, 8, 9, 10, 22, 23, 24, 25, 26,
		   38, 39, 40, 41, 42, 54, 55, 56, 57, 58},
	.oobfree = {{2, 4}, {11, 10}, {27, 10}, {43, 10}, {59, 5}, }
};

/* OOB description for 512 byte pages with 16 byte OOB */
static struct nand_ecclayout nandv2_hw_eccoob_smallpage = {
	.eccbytes = 1 * 9,
	.eccpos = {
		7,  8,  9, 10, 11, 12, 13, 14, 15
	},
	.oobfree = {
		{.offset = 0, .length = 5}
	}
};

/* OOB description for 2048 byte pages with 64 byte OOB */
static struct nand_ecclayout nandv2_hw_eccoob_largepage = {
	.eccbytes = 4 * 9,
	.eccpos = {
		 7,  8,  9, 10, 11, 12, 13, 14, 15,
		23, 24, 25, 26, 27, 28, 29, 30, 31,
		39, 40, 41, 42, 43, 44, 45, 46, 47,
		55, 56, 57, 58, 59, 60, 61, 62, 63
	},
	.oobfree = {
		{.offset = 2, .length = 4},
		{.offset = 16, .length = 7},
		{.offset = 32, .length = 7},
		{.offset = 48, .length = 7}
	}
};

static void __nand_boot_init memcpy32(void *trg, const void *src, int size)
{
	int i;
	unsigned int *t = trg;
	unsigned const int *s = src;

	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

/*
 * This function polls the NANDFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 *
 * @param       max_retries     number of retry attempts (separated by 1 us)
 * @param       param           parameter for debug
 */
static void __nand_boot_init wait_op_done(struct imx_nand_host *host)
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
	wait_op_done(host);
}

/*
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       islast  True if this is the last address cycle for command
 */
static void __nand_boot_init noinline send_addr(struct imx_nand_host *host, u16 addr)
{
	MTD_DEBUG(MTD_DEBUG_LEVEL3, "send_addr(host, 0x%x %d)\n", addr, islast);

	writew(addr, host->regs + NFC_FLASH_ADDR);
	writew(NFC_ADDR, host->regs + NFC_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host);
}

/*
 * This function requests the NANDFC to initate the transfer
 * of data currently in the NANDFC RAM buffer to the NAND device.
 *
 * @param	buf_id	      Specify Internal RAM Buffer number (0-3)
 * @param       spare_only    set true if only the spare area is transferred
 */
static void __nand_boot_init send_page(struct imx_nand_host *host,
		unsigned int ops)
{
	int bufs, i;

	if (nfc_is_v1() && host->pagesize_2k)
		bufs = 4;
	else
		bufs = 1;

	for (i = 0; i < bufs; i++) {
		/* NANDFC buffer 0 is used for page read/write */
		writew(i, host->regs + NFC_BUF_ADDR);

		writew(ops, host->regs + NFC_CONFIG2);

		/* Wait for operation to complete */
		wait_op_done(host);
	}
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
	wait_op_done(host);

	if (this->options & NAND_BUSWIDTH_16) {
		volatile u16 *mainbuf = host->main_area0;

		/*
		 * Pack the every-other-byte result for 16-bit ID reads
		 * into every-byte as the generic code expects and various
		 * chips implement.
		 */

		mainbuf[0] = (mainbuf[0] & 0xff) | ((mainbuf[1] & 0xff) << 8);
		mainbuf[1] = (mainbuf[2] & 0xff) | ((mainbuf[3] & 0xff) << 8);
		mainbuf[2] = (mainbuf[4] & 0xff) | ((mainbuf[5] & 0xff) << 8);
	}
	memcpy32(host->data_buf, host->main_area0, 16);
}

/*
 * This function requests the NANDFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static u16 get_dev_status(struct imx_nand_host *host)
{
	volatile u16 *mainbuf = host->main_area1;
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
	wait_op_done(host);

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
	u_char ret;

	/* Check for status request */
	if (host->status_request)
		return get_dev_status(host) & 0xFF;

	ret = *(uint8_t *)(host->data_buf + host->buf_start);
	host->buf_start++;

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
	uint16_t ret;

	ret = *(uint16_t *)(host->data_buf + host->buf_start);
	host->buf_start += 2;

	return ret;
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
	u16 col = host->buf_start;
	int n = mtd->oobsize + mtd->writesize - col;

	n = min(n, len);
	memcpy32(host->data_buf + col, buf, n);

	host->buf_start += n;
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
	u16 col = host->buf_start;
	int n = mtd->oobsize + mtd->writesize - col;

	n = min(n, len);

	memcpy32(buf, host->data_buf + col, len);

	host->buf_start += len;
}

/*
 * Function to transfer data to/from spare area.
 */
static void copy_spare(struct mtd_info *mtd, int bfrom)
{
	struct nand_chip *this = mtd->priv;
	struct imx_nand_host *host = this->priv;
	u16 i, j;
	u16 n = mtd->writesize >> 9;
	u8 *d = host->data_buf + mtd->writesize;
	u8 *s = host->spare0;
	u16 t = host->spare_len;

	j = (mtd->oobsize / n >> 1) << 1;

	if (bfrom) {
		for (i = 0; i < n - 1; i++)
			memcpy32(d + i * j, s + i * t, j);

		/* the last section */
		memcpy32(d + i * j, s + i * t, mtd->oobsize - i * j);
	} else {
		for (i = 0; i < n - 1; i++)
			memcpy32(&s[i * t], &d[i * j], j);

		/* the last section */
		memcpy32(&s[i * t], &d[i * j], mtd->oobsize - i * j);
	}
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

static void mxc_do_addr_cycle(struct mtd_info *mtd, int column, int page_addr)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

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
		host->buf_start = 0;
		host->status_request = 1;
		send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
		break;

	case NAND_CMD_READ0:
	case NAND_CMD_READOOB:
		if (command == NAND_CMD_READ0)
			host->buf_start = column;
		else
			host->buf_start = column + mtd->writesize;

		command = NAND_CMD_READ0;

		send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);

		if (host->pagesize_2k)
			/* send read confirm command */
			send_cmd(host, NAND_CMD_READSTART);

		send_page(host, NFC_OUTPUT);

		memcpy32(host->data_buf, host->main_area0, mtd->writesize);
		copy_spare(mtd, 1);
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
			host->buf_start = column;

			/* Set program pointer to spare region */
			if (!host->pagesize_2k)
				send_cmd(host, NAND_CMD_READOOB);
		} else {
			host->buf_start = column;

			/* Set program pointer to page start */
			if (!host->pagesize_2k)
				send_cmd(host, NAND_CMD_READ0);
		}
		send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);

		break;

	case NAND_CMD_PAGEPROG:
		memcpy32(host->main_area0, host->data_buf, mtd->writesize);
		copy_spare(mtd, 0);
		send_page(host, NFC_INPUT);
		send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
		break;

	case NAND_CMD_READID:
		send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
		host->buf_start = 0;
		send_read_id(host);
		break;

	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_RESET:
		send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
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
 * The generic flash bbt decriptors overlap with our ecc
 * hardware, so define some i.MX specific ones.
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
	.pattern = bbt_pattern,
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern,
};

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
	struct nand_ecclayout *oob_smallpage, *oob_largepage;
	u16 tmp;
	int err = 0;
#ifdef CONFIG_ARCH_IMX27
	PCCR1 |= PCCR1_NFC_BAUDEN;
#endif
	/* Allocate memory for MTD device structure and private data */
	host = kzalloc(sizeof(struct imx_nand_host) + NAND_MAX_PAGESIZE +
			NAND_MAX_OOBSIZE, GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	host->data_buf = (uint8_t *)(host + 1);
	host->base = (void __iomem *)dev->map_base;

	host->main_area0 = host->base;
	host->main_area1 = host->base + 0x200;

	if (nfc_is_v21()) {
		host->regs = host->base + 0x1000;
		host->spare0 = host->base + 0x1000;
		host->spare_len = 64;
		oob_smallpage = &nandv2_hw_eccoob_smallpage;
		oob_largepage = &nandv2_hw_eccoob_largepage;
	} else if (nfc_is_v1()) {
		host->regs = host->base;
		host->spare0 = host->base + 0x800;
		host->spare_len = 16;
		oob_smallpage = &nandv1_hw_eccoob_smallpage;
		oob_largepage = &nandv1_hw_eccoob_largepage;
	}

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

	tmp = readw(host->regs + NFC_CONFIG1);
	tmp |= NFC_INT_MSK;
	tmp &= ~NFC_SP_EN;
	if (nfc_is_v21())
		/* currently no support for 218 byte OOB with stronger ECC */
		tmp |= NFC_ECC_MODE;
	writew(tmp, host->regs + NFC_CONFIG1);

	if (pdata->hw_ecc) {
		this->ecc.calculate = imx_nand_calculate_ecc;
		this->ecc.hwctl = imx_nand_enable_hwecc;
		this->ecc.correct = imx_nand_correct_data;
		this->ecc.mode = NAND_ECC_HW;
		this->ecc.size = 512;
		tmp = readw(host->regs + NFC_CONFIG1);
		tmp |= NFC_ECC_EN;
		writew(tmp, host->regs + NFC_CONFIG1);
	} else {
		this->ecc.size = 512;
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
	if (nfc_is_v21()) {
		writew(0x0, host->regs + NFC_V21_UNLOCKSTART_BLKADDR);
		writew(0xffff, host->regs + NFC_V21_UNLOCKEND_BLKADDR);
		this->ecc.bytes = 9;
	} else if (nfc_is_v1()) {
		writew(0x0, host->regs + NFC_V1_UNLOCKSTART_BLKADDR);
		writew(0x4000, host->regs + NFC_V1_UNLOCKEND_BLKADDR);
		this->ecc.bytes = 3;
	}

	/* Unlock Block Command for given address range */
	writew(0x4, host->regs + NFC_WRPROT);

	this->ecc.layout = oob_smallpage;

	/* NAND bus width determines access funtions used by upper layer */
	if (pdata->width == 2) {
		this->options |= NAND_BUSWIDTH_16;
		this->ecc.layout = &nandv1_hw_eccoob_smallpage;
		imx_nand_set_layout(0, 16);
	}

	if (pdata->flash_bbt) {
		this->bbt_td = &bbt_main_descr;
		this->bbt_md = &bbt_mirror_descr;
		/* update flash based bbt */
		this->options |= NAND_USE_FLASH_BBT;
	}

	/* first scan to find the device and get the page size */
	if (nand_scan_ident(mtd, 1)) {
		err = -ENXIO;
		goto escan;
	}

	imx_nand_set_layout(mtd->writesize, pdata->width == 2 ? 16 : 8);

	if (mtd->writesize == 2048) {
		this->ecc.layout = oob_largepage;
		host->pagesize_2k = 1;
		if (nfc_is_v21()) {
			tmp = readw(host->regs + NFC_SPAS);
			tmp &= 0xff00;
			tmp |= NFC_SPAS_64;
			writew(tmp, host->regs + NFC_SPAS);
		}
	} else {
		if (nfc_is_v21()) {
			tmp = readw(host->regs + NFC_SPAS);
			tmp &= 0xff00;
			tmp |= NFC_SPAS_16;
			writew(tmp, host->regs + NFC_SPAS);
		}
	}

	/* second phase scan */
	if (nand_scan_tail(mtd)) {
		err = -ENXIO;
		goto escan;
	}

	add_mtd_device(mtd);

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
	if (host->pagesize_2k) {
		send_addr(host, offs & 0xff);
		send_addr(host, offs & 0xff);
		send_addr(host, (offs >> 11) & 0xff);
		send_addr(host, (offs >> 19) & 0xff);
		send_addr(host, (offs >> 27) & 0xff);
	} else {
		send_addr(host, offs & 0xff);
		send_addr(host, (offs >> 9) & 0xff);
		send_addr(host, (offs >> 17) & 0xff);
		send_addr(host, (offs >> 25) & 0xff);
	}
}

void __nand_boot_init imx_nand_load_image(void *dest, int size)
{
	struct imx_nand_host host;
	u32 tmp, page, block, blocksize, pagesize;

#ifdef CONFIG_ARCH_IMX27
	tmp = readl(IMX_SYSTEM_CTL_BASE + 0x14);
	if (tmp & (1 << 5))
		host.pagesize_2k = 1;
	else
		host.pagesize_2k = 0;
#endif
#ifdef CONFIG_ARCH_IMX31
	tmp = readl(IMX_CCM_BASE + CCM_RCSR);
	if (tmp & RCSR_NFMS)
		host.pagesize_2k = 1;
	else
		host.pagesize_2k = 0;
#endif
#ifdef CONFIG_ARCH_IMX35
	if (readl(IMX_CCM_BASE + CCM_RCSR) & (1 << 8))
		host.pagesize_2k = 1;
	else
		host.pagesize_2k = 0;
#endif
	if (host.pagesize_2k) {
		pagesize = 2048;
		blocksize = 128 * 1024;
	} else {
		pagesize = 512;
		blocksize = 16 * 1024;
	}

	host.base = (void __iomem *)IMX_NFC_BASE;
	if (nfc_is_v21()) {
		host.regs = host.base + 0x1000;
		host.spare0 = host.base + 0x1000;
		host.spare_len = 64;
	} else if (nfc_is_v1()) {
		host.regs = host.base;
		host.spare0 = host.base + 0x800;
		host.spare_len = 16;
	}

	send_cmd(&host, NAND_CMD_RESET);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	writew(0x2, host.regs + NFC_CONFIG);

	/* Unlock Block Command for given address range */
	writew(0x4, host.regs + NFC_WRPROT);

	tmp = readw(host.regs + NFC_CONFIG1);
	tmp |= NFC_ECC_EN | NFC_INT_MSK;
	if (nfc_is_v21())
		/* currently no support for 218 byte OOB with stronger ECC */
		tmp |= NFC_ECC_MODE;
	tmp &= ~NFC_SP_EN;
	writew(tmp, host.regs + NFC_CONFIG1);

	if (nfc_is_v21()) {
		if (host.pagesize_2k) {
			tmp = readw(host.regs + NFC_SPAS);
			tmp &= 0xff00;
			tmp |= NFC_SPAS_64;
			writew(tmp, host.regs + NFC_SPAS);
		} else {
			tmp = readw(host.regs + NFC_SPAS);
			tmp &= 0xff00;
			tmp |= NFC_SPAS_16;
			writew(tmp, host.regs + NFC_SPAS);
		}
	}

	block = page = 0;

	while (1) {
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
			if (host.pagesize_2k)
				send_cmd(&host, NAND_CMD_READSTART);
			send_page(&host, NFC_OUTPUT);
			page++;

			if (host.pagesize_2k) {
				if ((readw(host.spare0) & 0xff)
						!= 0xff)
					continue;
			} else {
				if ((readw(host.spare0 + 4) & 0xff00)
						!= 0xff00)
					continue;
			}

			memcpy32(dest, host.base, pagesize);
			dest += pagesize;
			size -= pagesize;

			if (size <= 0)
				return;
		}
		block++;
	}
}
#define CONFIG_NAND_IMX_BOOT_DEBUG
#ifdef CONFIG_NAND_IMX_BOOT_DEBUG
#include <command.h>

static int do_nand_boot_test(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	void *dest;
	int size;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	dest = (void *)strtoul_suffix(argv[1], NULL, 0);
	size = strtoul_suffix(argv[2], NULL, 0);

	imx_nand_load_image(dest, size);

	return 0;
}

static const __maybe_unused char cmd_nand_boot_test_help[] =
"Usage: nand_boot_test <dest> <size>\n";

BAREBOX_CMD_START(nand_boot_test)
	.cmd		= do_nand_boot_test,
	.usage		= "list a file or directory",
	BAREBOX_CMD_HELP(cmd_nand_boot_test_help)
BAREBOX_CMD_END
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
