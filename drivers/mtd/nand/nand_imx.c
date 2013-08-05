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
#include <io.h>
#include <of_mtd.h>
#include <errno.h>

#define NFC_V3_FLASH_CMD		(host->regs_axi + 0x00)
#define NFC_V3_FLASH_ADDR0		(host->regs_axi + 0x04)

#define NFC_V3_CONFIG1			(host->regs_axi + 0x34)
#define NFC_V3_CONFIG1_SP_EN		(1 << 0)
#define NFC_V3_CONFIG1_RBA(x)		(((x) & 0x7 ) << 4)

#define NFC_V3_ECC_STATUS_RESULT	(host->regs_axi + 0x38)

#define NFC_V3_LAUNCH			(host->regs_axi + 0x40)

#define NFC_V3_WRPROT			(host->regs_ip + 0x0)
#define NFC_V3_WRPROT_LOCK_TIGHT	(1 << 0)
#define NFC_V3_WRPROT_LOCK		(1 << 1)
#define NFC_V3_WRPROT_UNLOCK		(1 << 2)
#define NFC_V3_WRPROT_BLS_UNLOCK	(2 << 6)

#define NFC_V3_WRPROT_UNLOCK_BLK_ADD0   (host->regs_ip + 0x04)

#define NFC_V3_CONFIG2			(host->regs_ip + 0x24)
#define NFC_V3_CONFIG2_PS_512			(0 << 0)
#define NFC_V3_CONFIG2_PS_2048			(1 << 0)
#define NFC_V3_CONFIG2_PS_4096			(2 << 0)
#define NFC_V3_CONFIG2_ONE_CYCLE		(1 << 2)
#define NFC_V3_CONFIG2_ECC_EN			(1 << 3)
#define NFC_V3_CONFIG2_2CMD_PHASES		(1 << 4)
#define NFC_V3_CONFIG2_NUM_ADDR_PHASE0		(1 << 5)
#define NFC_V3_CONFIG2_ECC_MODE_8		(1 << 6)
#define NFC_V3_MX51_CONFIG2_PPB(x)		(((x) & 0x3) << 7)
#define NFC_V3_MX53_CONFIG2_PPB(x)		(((x) & 0x3) << 8)
#define NFC_V3_CONFIG2_NUM_ADDR_PHASE1(x)	(((x) & 0x3) << 12)
#define NFC_V3_CONFIG2_INT_MSK			(1 << 15)
#define NFC_V3_CONFIG2_ST_CMD(x)		(((x) & 0xff) << 24)
#define NFC_V3_CONFIG2_SPAS(x)			(((x) & 0xff) << 16)

#define NFC_V3_CONFIG3				(host->regs_ip + 0x28)
#define NFC_V3_CONFIG3_ADD_OP(x)		(((x) & 0x3) << 0)
#define NFC_V3_CONFIG3_FW8			(1 << 3)
#define NFC_V3_CONFIG3_SBB(x)			(((x) & 0x7) << 8)
#define NFC_V3_CONFIG3_NUM_OF_DEVICES(x)	(((x) & 0x7) << 12)
#define NFC_V3_CONFIG3_RBB_MODE			(1 << 15)
#define NFC_V3_CONFIG3_NO_SDMA			(1 << 20)

#define NFC_V3_IPC			(host->regs_ip + 0x2C)
#define NFC_V3_IPC_CREQ			(1 << 0)
#define NFC_V3_IPC_INT			(1 << 31)

#define NFC_V3_DELAY_LINE		(host->regs_ip + 0x34)

struct imx_nand_host {
	struct mtd_info		mtd;		
	struct nand_chip	nand;
	struct mtd_partition	*parts;
	struct device_d		*dev;

	void			*spare0;
	void			*main_area0;

	void __iomem		*base;
	void __iomem		*regs;
	void __iomem		*regs_axi;
	void __iomem		*regs_ip;
	int			status_request;
	struct clk		*clk;

	int			pagesize_2k;
	uint8_t			*data_buf;
	unsigned int		buf_start;
	int			spare_len;
	int			eccsize;

	int			hw_ecc;
	int			data_width;
	int			flash_bbt;

	void			(*preset)(struct mtd_info *);
	void			(*send_cmd)(struct imx_nand_host *, uint16_t);
	void			(*send_addr)(struct imx_nand_host *, uint16_t);
	void			(*send_page)(struct imx_nand_host *, unsigned int);
	void			(*send_read_id)(struct imx_nand_host *);
	void			(*send_read_param)(struct imx_nand_host *);
	uint16_t		(*get_dev_status)(struct imx_nand_host *);
	int			(*check_int)(struct imx_nand_host *);
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

/* OOB description for 4096 byte pages with 128 byte OOB */
static struct nand_ecclayout nandv2_hw_eccoob_4k = {
	.eccbytes = 8 * 9,
	.eccpos = {
		7,  8,  9, 10, 11, 12, 13, 14, 15,
		23, 24, 25, 26, 27, 28, 29, 30, 31,
		39, 40, 41, 42, 43, 44, 45, 46, 47,
		55, 56, 57, 58, 59, 60, 61, 62, 63,
		71, 72, 73, 74, 75, 76, 77, 78, 79,
		87, 88, 89, 90, 91, 92, 93, 94, 95,
		103, 104, 105, 106, 107, 108, 109, 110, 111,
		119, 120, 121, 122, 123, 124, 125, 126, 127,
	},
	.oobfree = {
		{.offset = 2, .length = 4},
		{.offset = 16, .length = 7},
		{.offset = 32, .length = 7},
		{.offset = 48, .length = 7},
		{.offset = 64, .length = 7},
		{.offset = 80, .length = 7},
		{.offset = 96, .length = 7},
		{.offset = 112, .length = 7},
	}
};

static void memcpy32(void *trg, const void *src, int size)
{
	int i;
	unsigned int *t = trg;
	unsigned const int *s = src;

#ifdef CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS
	if (!((unsigned long)trg & 0x3) && !((unsigned long)src & 0x3))
		memcpy(trg, src, size);
	else
#endif
	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

static int check_int_v3(struct imx_nand_host *host)
{
	uint32_t tmp;

	tmp = readl(NFC_V3_IPC);
	if (!(tmp & NFC_V3_IPC_INT))
		return 0;

	tmp &= ~NFC_V3_IPC_INT;
	writel(tmp, NFC_V3_IPC);

	return 1;
}

static int check_int_v1_v2(struct imx_nand_host *host)
{
	uint32_t tmp;

	tmp = readw(host->regs + NFC_V1_V2_CONFIG2);
	if (!(tmp & NFC_V1_V2_CONFIG2_INT))
		return 0;

	writew(tmp & ~NFC_V1_V2_CONFIG2_INT, host->regs + NFC_V1_V2_CONFIG2);

	return 1;
}

static void wait_op_done(struct imx_nand_host *host)
{
	int i;

	/* This is a timeout of roughly 15ms on my system. We
	 * need about 2us, but be generous. Don't use udelay
	 * here as we might be here from nand booting.
	 */
	for (i = 0; i < 100000; i++) {
		if (host->check_int(host))
			return;
	}
}

/*
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 */
static void send_cmd_v3(struct imx_nand_host *host, uint16_t cmd)
{
	/* fill command */
	writel(cmd, NFC_V3_FLASH_CMD);

	/* send out command */
	writel(NFC_CMD, NFC_V3_LAUNCH);

	/* Wait for operation to complete */
	wait_op_done(host);
}

static void send_cmd_v1_v2(struct imx_nand_host *host, u16 cmd)
{
	MTD_DEBUG(MTD_DEBUG_LEVEL3, "send_cmd(host, 0x%x)\n", cmd);

	writew(cmd, host->regs + NFC_V1_V2_FLASH_CMD);
	writew(NFC_CMD, host->regs + NFC_V1_V2_CONFIG2);

	if (cpu_is_mx21() && (cmd == NAND_CMD_RESET)) {
		/* Reset completion is indicated by NFC_CONFIG2 */
		/* being set to 0 */
		int i;
		for (i = 0; i < 100000; i++) {
			if (readw(host->regs + NFC_V1_V2_CONFIG2) == 0) {
				break;
			}
		}
	} else
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
static void send_addr_v3(struct imx_nand_host *host, uint16_t addr)
{
	/* fill address */
	writel(addr, NFC_V3_FLASH_ADDR0);

	/* send out address */
	writel(NFC_ADDR, NFC_V3_LAUNCH);

	wait_op_done(host);
}

static void send_addr_v1_v2(struct imx_nand_host *host, u16 addr)
{
	MTD_DEBUG(MTD_DEBUG_LEVEL3, "send_addr(host, 0x%x %d)\n", addr, islast);

	writew(addr, host->regs + NFC_V1_V2_FLASH_ADDR);
	writew(NFC_ADDR, host->regs + NFC_V1_V2_CONFIG2);

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
static void send_page_v3(struct imx_nand_host *host, unsigned int ops)
{
	uint32_t tmp;

	tmp = readl(NFC_V3_CONFIG1);
	tmp &= ~(7 << 4);
	writel(tmp, NFC_V3_CONFIG1);

	/* transfer data from NFC ram to nand */
	writel(ops, NFC_V3_LAUNCH);

	wait_op_done(host);
}

static void send_page_v1_v2(struct imx_nand_host *host,
		unsigned int ops)
{
	int bufs, i;

	if (nfc_is_v1() && host->pagesize_2k)
		bufs = 4;
	else
		bufs = 1;

	for (i = 0; i < bufs; i++) {
		/* NANDFC buffer 0 is used for page read/write */
		writew(i, host->regs + NFC_V1_V2_BUF_ADDR);

		writew(ops, host->regs + NFC_V1_V2_CONFIG2);

		/* Wait for operation to complete */
		wait_op_done(host);
	}
}

/*
 * This function requests the NANDFC to perform a read of the
 * NAND device ID.
 */
static void send_read_id_v3(struct imx_nand_host *host)
{
	/* Read ID into main buffer */
	writel(NFC_ID, NFC_V3_LAUNCH);

	wait_op_done(host);

	memcpy(host->data_buf, host->main_area0, 16);
}

static void send_read_param_v3(struct imx_nand_host *host)
{
	/* Read ID into main buffer */
	writel(NFC_OUTPUT, NFC_V3_LAUNCH);

	wait_op_done(host);

	memcpy(host->data_buf, host->main_area0, 1024);
}

static void send_read_id_v1_v2(struct imx_nand_host *host)
{
	struct nand_chip *this = &host->nand;

	/* NANDFC buffer 0 is used for device ID output */
	writew(0x0, host->regs + NFC_V1_V2_BUF_ADDR);

	writew(NFC_ID, host->regs + NFC_V1_V2_CONFIG2);

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

/* FIXME : to check on real HW */
static void send_read_param_v1_v2(struct imx_nand_host *host)
{
	struct nand_chip *this = &host->nand;

	/* NANDFC buffer 0 is used for device ID output */
	writew(0x0, host->regs + NFC_V1_V2_BUF_ADDR);

	writew(NFC_OUTPUT, host->regs + NFC_V1_V2_CONFIG2);

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
	memcpy32(host->data_buf, host->main_area0, 1024);
}
/*
 * This function requests the NANDFC to perform a read of the
 * NAND device status and returns the current status.
 *
 * @return  device status
 */
static uint16_t get_dev_status_v3(struct imx_nand_host *host)
{
	writew(NFC_STATUS, NFC_V3_LAUNCH);
	wait_op_done(host);

	return readl(NFC_V3_CONFIG1) >> 16;
}

static u16 get_dev_status_v1_v2(struct imx_nand_host *host)
{
	void *main_buf = host->main_area0;
	u32 store;
	u16 ret;

	writew(0x0, host->regs + NFC_V1_V2_BUF_ADDR);

	/*
	 * The device status is stored in main_area0. To
	 * prevent corruption of the buffer save the value
	 * and restore it afterwards.
	 */
	store = readl(main_buf);

	writew(NFC_STATUS, host->regs + NFC_V1_V2_CONFIG2);

	/* Wait for operation to complete */
	wait_op_done(host);

	/* Status is placed in first word of main buffer */
	/* get status, then recovery area 1 data */
	ret = readw(main_buf);

	writel(store, main_buf);

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

static int imx_nand_correct_data_v1(struct mtd_info *mtd, u_char * dat,
				 u_char * read_ecc, u_char * calc_ecc)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;

	/*
	 * 1-Bit errors are automatically corrected in HW.  No need for
	 * additional correction.  2-Bit errors cannot be corrected by
	 * HW ECC, so we need to return failure
	 */
	u16 ecc_status = readw(host->regs + NFC_V1_ECC_STATUS_RESULT);

	if (((ecc_status & 0x3) == 2) || ((ecc_status >> 2) == 2)) {
		MTD_DEBUG(MTD_DEBUG_LEVEL0,
		      "MXC_NAND: HWECC uncorrectable 2-bit ECC error\n");
		return -1;
	}

	return 0;
}

static int imx_nand_correct_data_v2_v3(struct mtd_info *mtd, u_char *dat,
				u_char *read_ecc, u_char *calc_ecc)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;
	u32 ecc_stat, err;
	int no_subpages = 1;
	int ret = 0;
	u8 ecc_bit_mask, err_limit;

	ecc_bit_mask = (host->eccsize == 4) ? 0x7 : 0xf;
	err_limit = (host->eccsize == 4) ? 0x4 : 0x8;

	no_subpages = mtd->writesize >> 9;

	if (nfc_is_v21())
		ecc_stat = readl(host->regs + NFC_V2_ECC_STATUS_RESULT1);
	else
		ecc_stat = readl(NFC_V3_ECC_STATUS_RESULT);

	do {
		err = ecc_stat & ecc_bit_mask;
		if (err > err_limit) {
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
		return host->get_dev_status(host) & 0xFF;

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
	memcpy(host->data_buf + col, buf, n);

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

	/* handle the read param special case */
	if ((mtd->writesize == 0) && (len != 0))
		n = len;

	memcpy(buf, host->data_buf + col, n);

	host->buf_start += n;
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
		 *
		 * The colum address must be sent to the flash in
		 * order to get the ONFI header (0x20)
		 */
		host->send_addr(host, column);
		if (host->pagesize_2k)
			/* another col addr cycle for 2k page */
			host->send_addr(host, 0);
	}

	/*
	 * Write out page address, if necessary
	 */
	if (page_addr != -1) {
		host->send_addr(host, (page_addr & 0xff));	/* paddr_0 - p_addr_7 */

		if (host->pagesize_2k) {
			host->send_addr(host, (page_addr >> 8) & 0xFF);
			if (mtd->size >= 0x10000000) {
				host->send_addr(host, (page_addr >> 16) & 0xff);
			}
		} else {
			/* One more address cycle for higher density devices */
			if (mtd->size >= 0x4000000) {
				/* paddr_8 - paddr_15 */
				host->send_addr(host, (page_addr >> 8) & 0xff);
				host->send_addr(host, (page_addr >> 16) & 0xff);
			} else
				/* paddr_8 - paddr_15 */
				host->send_addr(host, (page_addr >> 8) & 0xff);
		}
	}
}

/*
 * v2 and v3 type controllers can do 4bit or 8bit ecc depending
 * on how much oob the nand chip has. For 8bit ecc we need at least
 * 26 bytes of oob data per 512 byte block.
 */
static int get_eccsize(struct mtd_info *mtd)
{
	int oobbytes_per_512 = 0;

	oobbytes_per_512 = mtd->oobsize * 512 / mtd->writesize;

	if (oobbytes_per_512 < 26)
		return 4;
	else
		return 8;
}

static void preset_v1_v2(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct imx_nand_host *host = nand_chip->priv;
	uint16_t config1 = 0;

	if (nand_chip->ecc.mode == NAND_ECC_HW)
		config1 |= NFC_V1_V2_CONFIG1_ECC_EN;

	if (nfc_is_v21())
		config1 |= NFC_V2_CONFIG1_FP_INT;

	if (nfc_is_v21() && mtd->writesize) {
		uint16_t pages_per_block = mtd->erasesize / mtd->writesize;

		host->eccsize = get_eccsize(mtd);
		if (host->eccsize == 4)
			config1 |= NFC_V2_CONFIG1_ECC_MODE_4;

		config1 |= NFC_V2_CONFIG1_PPB(ffs(pages_per_block) - 6);
	} else {
		host->eccsize = 1;
	}

	writew(config1, host->regs + NFC_V1_V2_CONFIG1);
	/* preset operation */

	/* Unlock the internal RAM Buffer */
	writew(0x2, host->regs + NFC_V1_V2_CONFIG);

	/* Blocks to be unlocked */
	if (nfc_is_v21()) {
		writew(0x0, host->regs + NFC_V21_UNLOCKSTART_BLKADDR);
		writew(0xffff, host->regs + NFC_V21_UNLOCKEND_BLKADDR);
	} else if (nfc_is_v1()) {
		writew(0x0, host->regs + NFC_V1_UNLOCKSTART_BLKADDR);
		writew(0x4000, host->regs + NFC_V1_UNLOCKEND_BLKADDR);
	} else
		BUG();

	/* Unlock Block Command for given address range */
	writew(0x4, host->regs + NFC_V1_V2_WRPROT);
}

static void preset_v3(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct imx_nand_host *host = chip->priv;
	uint32_t config2, config3;
	int i, addr_phases;

	writel(NFC_V3_CONFIG1_RBA(0), NFC_V3_CONFIG1);
	writel(NFC_V3_IPC_CREQ, NFC_V3_IPC);

	/* Unlock the internal RAM Buffer */
	writel(NFC_V3_WRPROT_BLS_UNLOCK | NFC_V3_WRPROT_UNLOCK,
			NFC_V3_WRPROT);

	/* Blocks to be unlocked */
	for (i = 0; i < NAND_MAX_CHIPS; i++)
		writel(0x0 | (0xffff << 16),
				NFC_V3_WRPROT_UNLOCK_BLK_ADD0 + (i << 2));

	writel(0, NFC_V3_IPC);

	/* if the flash has a 224 oob, the NFC must be configured to 218 */
	config2 = NFC_V3_CONFIG2_ONE_CYCLE |
		NFC_V3_CONFIG2_2CMD_PHASES |
		NFC_V3_CONFIG2_SPAS(((mtd->oobsize > 218) ?
			218 : mtd->oobsize) >> 1) |
		NFC_V3_CONFIG2_ST_CMD(0x70) |
		NFC_V3_CONFIG2_NUM_ADDR_PHASE0;

	if (chip->ecc.mode == NAND_ECC_HW)
		config2 |= NFC_V3_CONFIG2_ECC_EN;

	addr_phases = fls(chip->pagemask) >> 3;

	if (mtd->writesize == 2048) {
		config2 |= NFC_V3_CONFIG2_PS_2048;
		config2 |= NFC_V3_CONFIG2_NUM_ADDR_PHASE1(addr_phases);
	} else if (mtd->writesize == 4096) {
		config2 |= NFC_V3_CONFIG2_PS_4096;
		config2 |= NFC_V3_CONFIG2_NUM_ADDR_PHASE1(addr_phases);
	} else {
		config2 |= NFC_V3_CONFIG2_PS_512;
		config2 |= NFC_V3_CONFIG2_NUM_ADDR_PHASE1(addr_phases - 1);
	}

	if (mtd->writesize) {
		if (cpu_is_mx51())
			config2 |= NFC_V3_MX51_CONFIG2_PPB(
					ffs(mtd->erasesize / mtd->writesize) - 6);
		else
			config2 |= NFC_V3_MX53_CONFIG2_PPB(
					ffs(mtd->erasesize / mtd->writesize) - 6);
		host->eccsize = get_eccsize(mtd);
		if (host->eccsize == 8)
			config2 |= NFC_V3_CONFIG2_ECC_MODE_8;
	}

	writel(config2, NFC_V3_CONFIG2);

	config3 = NFC_V3_CONFIG3_NUM_OF_DEVICES(0) |
			NFC_V3_CONFIG3_NO_SDMA |
			NFC_V3_CONFIG3_RBB_MODE |
			NFC_V3_CONFIG3_SBB(6) | /* Reset default */
			NFC_V3_CONFIG3_ADD_OP(0);

	if (!(chip->options & NAND_BUSWIDTH_16))
		config3 |= NFC_V3_CONFIG3_FW8;

	writel(config3, NFC_V3_CONFIG3);

	writel(0, NFC_V3_DELAY_LINE);
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
	case NAND_CMD_RESET:
		host->preset(mtd);
		host->send_cmd(host, command);
		break;

	case NAND_CMD_STATUS:
		host->buf_start = 0;
		host->status_request = 1;
		host->send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
		break;

	case NAND_CMD_READ0:
	case NAND_CMD_READOOB:
		if (command == NAND_CMD_READ0)
			host->buf_start = column;
		else
			host->buf_start = column + mtd->writesize;

		command = NAND_CMD_READ0;

		host->send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);

		if (host->pagesize_2k)
			/* send read confirm command */
			host->send_cmd(host, NAND_CMD_READSTART);

		host->send_page(host, NFC_OUTPUT);

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
				host->send_cmd(host, NAND_CMD_READOOB);
		} else {
			host->buf_start = column;

			/* Set program pointer to page start */
			if (!host->pagesize_2k)
				host->send_cmd(host, NAND_CMD_READ0);
		}
		host->send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);

		break;

	case NAND_CMD_PAGEPROG:
		memcpy32(host->main_area0, host->data_buf, mtd->writesize);
		copy_spare(mtd, 0);
		host->send_page(host, NFC_INPUT);
		host->send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
		break;

	case NAND_CMD_READID:
		host->send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
		host->send_read_id(host);
		host->buf_start = 0;
		break;

	case NAND_CMD_PARAM:
		host->send_cmd(host, command);
		mxc_do_addr_cycle(mtd, column, page_addr);
		host->send_read_param(host);
		host->buf_start = 0;
		break;

	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
		host->send_cmd(host, command);
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
	.options = NAND_BBT_LASTBLOCK
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = bbt_pattern,
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = mirror_pattern,
};

static int __init mxcnd_probe_dt(struct imx_nand_host *host)
{
	struct device_node *np = host->dev->device_node;
	int buswidth;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return 1;

	if (!np)
		return 1;

	if (of_get_nand_ecc_mode(np) == NAND_ECC_HW)
		host->hw_ecc = 1;

	host->flash_bbt = of_get_nand_on_flash_bbt(np);

	buswidth = of_get_nand_bus_width(np);
	if (buswidth < 0)
		return buswidth;

	host->data_width = buswidth / 8;

	return 0;
}

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
	struct imx_nand_host *host;
	struct nand_ecclayout *oob_smallpage, *oob_largepage, *oob_4kpage;
	int err = 0;

	/* Allocate memory for MTD device structure and private data */
	host = kzalloc(sizeof(struct imx_nand_host) + NAND_MAX_PAGESIZE +
			NAND_MAX_OOBSIZE, GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	host->dev = dev;

	err = mxcnd_probe_dt(host);
	if (err < 0)
		goto escan;

	if (err > 0) {
		struct imx_nand_platform_data *pdata;

		pdata = dev->platform_data;
		host->flash_bbt = pdata->flash_bbt;
		host->data_width = pdata->width;
		host->hw_ecc = pdata->hw_ecc;
	}

	host->data_buf = (uint8_t *)(host + 1);

	if (nfc_is_v1() || nfc_is_v21()) {
		host->preset = preset_v1_v2;
		host->send_cmd = send_cmd_v1_v2;
		host->send_addr = send_addr_v1_v2;
		host->send_page = send_page_v1_v2;
		host->send_read_id = send_read_id_v1_v2;
		host->send_read_param = send_read_param_v1_v2; /* FIXME : to check */
		host->get_dev_status = get_dev_status_v1_v2;
		host->check_int = check_int_v1_v2;
	}

	if (nfc_is_v21()) {
		host->base = dev_request_mem_region(dev, 0);
		host->main_area0 = host->base;
		host->regs = host->base + 0x1e00;
		host->spare0 = host->base + 0x1000;
		host->spare_len = 64;
		oob_smallpage = &nandv2_hw_eccoob_smallpage;
		oob_largepage = &nandv2_hw_eccoob_largepage;
		oob_4kpage = &nandv2_hw_eccoob_4k; /* FIXME : to check */
	} else if (nfc_is_v1()) {
		host->base = dev_request_mem_region(dev, 0);
		host->main_area0 = host->base;
		host->regs = host->base + 0xe00;
		host->spare0 = host->base + 0x800;
		host->spare_len = 16;
		oob_smallpage = &nandv1_hw_eccoob_smallpage;
		oob_largepage = &nandv1_hw_eccoob_largepage;
		oob_4kpage = &nandv1_hw_eccoob_smallpage; /* FIXME : to check  */
	} else if (nfc_is_v3_2()) {
		host->regs_ip = dev_request_mem_region(dev, 0);
		host->base = dev_request_mem_region(dev, 1);
		host->main_area0 = host->base;

		if (!host->regs_ip) {
			dev_err(dev, "no second mem region\n");
			err = -ENODEV;
			goto escan;
		}

		host->regs_axi = host->base + 0x1e00;
		host->spare0 = host->base + 0x1000;
		host->spare_len = 64;
		host->preset = preset_v3;
		host->send_cmd = send_cmd_v3;
		host->send_addr = send_addr_v3;
		host->send_page = send_page_v3;
		host->send_read_id = send_read_id_v3;
		host->send_read_param = send_read_param_v3;
		host->get_dev_status = get_dev_status_v3;
		host->check_int = check_int_v3;
		oob_smallpage = &nandv2_hw_eccoob_smallpage;
		oob_largepage = &nandv2_hw_eccoob_largepage;
		oob_4kpage = &nandv2_hw_eccoob_4k;
	} else {
		err = -EINVAL;
		goto escan;
	}

	/* structures must be linked */
	this = &host->nand;
	mtd = &host->mtd;
	mtd->priv = this;
	mtd->parent = dev;
	mtd->name = "imx_nand";

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

	if (host->hw_ecc) {
		this->ecc.calculate = imx_nand_calculate_ecc;
		this->ecc.hwctl = imx_nand_enable_hwecc;
		if (nfc_is_v1())
			this->ecc.correct = imx_nand_correct_data_v1;
		else
			this->ecc.correct = imx_nand_correct_data_v2_v3;
		this->ecc.mode = NAND_ECC_HW;
		this->ecc.size = 512;
	} else {
		this->ecc.size = 512;
		this->ecc.mode = NAND_ECC_SOFT;
	}

	this->ecc.layout = oob_smallpage;

	/* NAND bus width determines access functions used by upper layer */
	if (host->data_width == 2) {
		this->options |= NAND_BUSWIDTH_16;
		this->ecc.layout = &nandv1_hw_eccoob_smallpage;
		imx_nand_set_layout(0, 16);
	}

	if (host->flash_bbt) {
		this->bbt_td = &bbt_main_descr;
		this->bbt_md = &bbt_mirror_descr;
		/* update flash based bbt */
		this->bbt_options |= NAND_BBT_USE_FLASH;
	}

	/* first scan to find the device and get the page size */
	if (nand_scan_ident(mtd, 1, NULL)) {
		err = -ENXIO;
		goto escan;
	}

	/* Call preset again, with correct writesize this time */
	host->preset(mtd);

	imx_nand_set_layout(mtd->writesize, host->data_width == 2 ? 16 : 8);

	if (mtd->writesize >= 2048) {
		if (!host->flash_bbt)
			dev_warn(dev, "2k or 4k flash detected without flash_bbt. "
					"You will loose factory bad block markers!\n");

		if (mtd->writesize == 2048)
			this->ecc.layout = oob_largepage;
		else
			this->ecc.layout = oob_4kpage;
		host->pagesize_2k = 1;
		if (nfc_is_v21())
			writew(NFC_V2_SPAS_SPARESIZE(64), host->regs + NFC_V2_SPAS);
	} else {
		bbt_main_descr.options |= NAND_BBT_WRITE | NAND_BBT_CREATE;
		bbt_mirror_descr.options |= NAND_BBT_WRITE | NAND_BBT_CREATE;

		if (nfc_is_v21())
			writew(NFC_V2_SPAS_SPARESIZE(16), host->regs + NFC_V2_SPAS);
	}

	if (this->ecc.mode == NAND_ECC_HW)
		this->ecc.strength = host->eccsize;

	/* second phase scan */
	if (nand_scan_tail(mtd)) {
		err = -ENXIO;
		goto escan;
	}

	if (host->flash_bbt && this->bbt_td->pages[0] == -1 && this->bbt_md->pages[0] == -1) {
		dev_warn(dev, "no BBT found. create one using the imx_nand_bbm command\n");
	} else {
		bbt_main_descr.options |= NAND_BBT_WRITE | NAND_BBT_CREATE;
		bbt_mirror_descr.options |= NAND_BBT_WRITE | NAND_BBT_CREATE;
	}

	add_mtd_nand_device(mtd, "nand");

	dev->priv = host;

	return 0;

escan:
	kfree(host);

	return err;

}

static __maybe_unused struct of_device_id imx_nand_compatible[] = {
	{
		.compatible = "fsl,imx21-nand",
	}, {
		.compatible = "fsl,imx25-nand",
	}, {
		.compatible = "fsl,imx27-nand",
	}, {
		.compatible = "fsl,imx51-nand",
	}, {
		.compatible = "fsl,imx53-nand",
	}, {
		/* sentinel */
	}
};

static struct driver_d imx_nand_driver = {
	.name  = "imx_nand",
	.probe = imxnd_probe,
	.of_compatible = DRV_OF_COMPAT(imx_nand_compatible),
};
device_platform_driver(imx_nand_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC NAND MTD driver");
MODULE_LICENSE("GPL");
