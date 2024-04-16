// SPDX-License-Identifier: GPL-2.0-only
/*
 * drivers/mtd/nand/mrvl_nand.c
 *
 * Copyright © 2005 Intel Corporation
 * Copyright © 2006 Marvell International Ltd.
 * Copyright (C) 2014 Robert Jarzmik
 *
 * See Documentation/mtd/nand/pxa3xx-nand.txt for more details.
 */
#include <common.h>

#include <driver.h>
#include <dma/apbh-dma.h>
#include <errno.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/nand.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <malloc.h>
#include <of_mtd.h>
#include <stmp-device.h>

#include <platform_data/mtd-nand-mrvl.h>

#define	CHIP_DELAY_TIMEOUT_US	500000
#define PAGE_CHUNK_SIZE		(2048)

/*
 * Define a buffer size for the initial command that detects the flash device:
 * STATUS, READID and PARAM. The largest of these is the PARAM command,
 * needing 256 bytes.
 */
#define INIT_BUFFER_SIZE	256

/* registers and bit definitions */
#define NDCR		(0x00) /* Control register */
#define NDTR0CS0	(0x04) /* Timing Parameter 0 for CS0 */
#define NDTR1CS0	(0x0C) /* Timing Parameter 1 for CS0 */
#define NDSR		(0x14) /* Status Register */
#define NDPCR		(0x18) /* Page Count Register */
#define NDBDR0		(0x1C) /* Bad Block Register 0 */
#define NDBDR1		(0x20) /* Bad Block Register 1 */
#define NDECCCTRL	(0x28) /* ECC control */
#define NDDB		(0x40) /* Data Buffer */
#define NDCB0		(0x48) /* Command Buffer0 */
#define NDCB1		(0x4C) /* Command Buffer1 */
#define NDCB2		(0x50) /* Command Buffer2 */
#define NDCB3		(0x54) /* Command Buffer3 */

#define NDCR_SPARE_EN		(0x1 << 31)
#define NDCR_ECC_EN		(0x1 << 30)
#define NDCR_DMA_EN		(0x1 << 29)
#define NDCR_ND_RUN		(0x1 << 28)
#define NDCR_DWIDTH_C		(0x1 << 27)
#define NDCR_DWIDTH_M		(0x1 << 26)
#define NDCR_PAGE_SZ		(0x1 << 24)
#define NDCR_NCSX		(0x1 << 23)
#define NDCR_ND_MODE		(0x3 << 21)
#define NDCR_NAND_MODE		(0x0)
#define NDCR_CLR_PG_CNT		(0x1 << 20)
#define NDCR_STOP_ON_UNCOR	(0x1 << 19)
#define NDCR_RD_ID_CNT_MASK	(0x7 << 16)
#define NDCR_RD_ID_CNT(x)	(((x) << 16) & NDCR_RD_ID_CNT_MASK)

#define NDCR_RA_START		(0x1 << 15)
#define NDCR_PG_PER_BLK		(0x1 << 14)
#define NDCR_ND_ARB_EN		(0x1 << 12)
#define NDCR_INT_MASK           (0xFFF)

#define NDSR_MASK		(0xfff)
#define NDSR_ERR_CNT_OFF	(16)
#define NDSR_ERR_CNT_MASK       (0x1f)
#define NDSR_ERR_CNT(sr)	((sr >> NDSR_ERR_CNT_OFF) & NDSR_ERR_CNT_MASK)
#define NDSR_RDY                (0x1 << 12)
#define NDSR_FLASH_RDY          (0x1 << 11)
#define NDSR_CS0_PAGED		(0x1 << 10)
#define NDSR_CS1_PAGED		(0x1 << 9)
#define NDSR_CS0_CMDD		(0x1 << 8)
#define NDSR_CS1_CMDD		(0x1 << 7)
#define NDSR_CS0_BBD		(0x1 << 6)
#define NDSR_CS1_BBD		(0x1 << 5)
#define NDSR_UNCORERR		(0x1 << 4)
#define NDSR_CORERR		(0x1 << 3)
#define NDSR_WRDREQ		(0x1 << 2)
#define NDSR_RDDREQ		(0x1 << 1)
#define NDSR_WRCMDREQ		(0x1)

#define NDECCCTRL_BCH_EN	BIT(0)

#define NDCB0_LEN_OVRD		(0x1 << 28)
#define NDCB0_ST_ROW_EN         (0x1 << 26)
#define NDCB0_AUTO_RS		(0x1 << 25)
#define NDCB0_CSEL		(0x1 << 24)
#define NDCB0_EXT_CMD_TYPE_MASK	(0x7 << 29)
#define NDCB0_EXT_CMD_TYPE(x)	(((x) << 29) & NDCB0_EXT_CMD_TYPE_MASK)
#define NDCB0_CMD_TYPE_MASK	(0x7 << 21)
#define NDCB0_CMD_TYPE(x)	(((x) << 21) & NDCB0_CMD_TYPE_MASK)
#define NDCB0_NC		(0x1 << 20)
#define NDCB0_DBC		(0x1 << 19)
#define NDCB0_ADDR_CYC_MASK	(0x7 << 16)
#define NDCB0_ADDR_CYC(x)	(((x) << 16) & NDCB0_ADDR_CYC_MASK)
#define NDCB0_CMD2_MASK		(0xff << 8)
#define NDCB0_CMD1_MASK		(0xff)
#define NDCB0_ADDR_CYC_SHIFT	(16)

#define EXT_CMD_TYPE_DISPATCH	6 /* Command dispatch */
#define EXT_CMD_TYPE_NAKED_RW	5 /* Naked read or Naked write */
#define EXT_CMD_TYPE_READ	4 /* Read */
#define EXT_CMD_TYPE_DISP_WR	4 /* Command dispatch with write */
#define EXT_CMD_TYPE_FINAL	3 /* Final command */
#define EXT_CMD_TYPE_LAST_RW	1 /* Last naked read/write */
#define EXT_CMD_TYPE_MONO	0 /* Monolithic read/write */

/* macros for registers read/write */
#define nand_writel(host, off, val) \
	_nand_writel(__func__, __LINE__, (host), (off), (val))

#define nand_writesl(host, off, buf, nbbytes)		\
	writesl((host)->mmio_base + (off), buf, nbbytes)

#define nand_readl(host, off)	\
	_nand_readl(__func__, __LINE__, (host), (off))

#define nand_readsl(host, off, buf, nbbytes)		\
	readsl((host)->mmio_base + (off), buf, nbbytes)

struct mrvl_nand_variant {
	unsigned int	hwflags;
};

struct mrvl_nand_host {
	struct nand_chip	chip;
	struct mtd_partition	*parts;
	struct device		*dev;
	struct clk		*core_clk;

	/* calculated from mrvl_nand_flash data */
	unsigned int		col_addr_cycles;
	unsigned int		row_addr_cycles;
	size_t			read_id_bytes;

	void __iomem		*mmio_base;
	unsigned int		hwflags;
#define HWFLAGS_ECC_BCH		BIT(0)
#define HWFLAGS_HAS_NDCB3	BIT(1)

	unsigned int		buf_start;
	unsigned int		buf_count;
	unsigned int		buf_size;

	unsigned char		*data_buff;

	int			keep_config;
	int			ecc_strength;
	int			ecc_step;

	int			num_cs;		/* avaiable CS signals */
	int			cs;		/* selected chip 0/1 */
	int			use_ecc;	/* use HW ECC ? */
	int			ecc_bch;	/* HW ECC is BCH */
	int			use_spare;	/* use spare ? */
	int			flash_bbt;

	unsigned int		data_size;	/* data to be read from FIFO */
	unsigned int		chunk_size;	/* split commands chunk size */
	unsigned int		oob_size;
	unsigned int		spare_size;
	unsigned int		ecc_size;
	unsigned int		max_bitflips;
	int			cmd_ongoing;

	/* cached register value */
	uint32_t		reg_ndcr;
	uint32_t		ndtr0cs0_chip0;
	uint32_t		ndtr1cs0_chip0;
	uint32_t		ndtr0cs0_chip1;
	uint32_t		ndtr1cs0_chip1;

	/* generated NDCBx register values */
	uint32_t		ndcb0;
	uint32_t		ndcb1;
	uint32_t		ndcb2;
	uint32_t		ndcb3;
};

static u8 bbt_pattern[] = {'M', 'V', 'B', 'b', 't', '0' };
static u8 bbt_mirror_pattern[] = {'1', 't', 'b', 'B', 'V', 'M' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = bbt_pattern,
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		/* Last 8 blocks in each chip */
	.pattern = bbt_mirror_pattern,
};

static struct nand_ecclayout ecc_layout_512B_hwecc = {
	.eccbytes = 6,
	.eccpos = {
		8, 9, 10, 11, 12, 13, 14, 15 },
	.oobfree = { {0, 8} }
};

static struct nand_ecclayout ecc_layout_2KB_hwecc = {
	.eccbytes = 24,
	.eccpos = {
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63 },
	.oobfree = { {0, 40} }
};

static struct nand_ecclayout ecc_layout_2KB_bch4bit = {
	.eccbytes = 32,
	.eccpos = {
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 30} }
};

static struct nand_ecclayout ecc_layout_4KB_bch4bit = {
	.eccbytes = 64,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63,
		96,  97,  98,  99,  100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26}, {64, 32} }
};

static struct nand_ecclayout ecc_layout_4KB_bch8bit = {
	.eccbytes = 64,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63},
	/* Bootrom looks in bytes 0 & 5 for bad blocks */
	.oobfree = { {1, 4}, {6, 26} }
};

#define NDTR0_tCH(c)	(min((c), 7) << 19)
#define NDTR0_tCS(c)	(min((c), 7) << 16)
#define NDTR0_tWH(c)	(min((c), 7) << 11)
#define NDTR0_tWP(c)	(min((c), 7) << 8)
#define NDTR0_tRH(c)	(min((c), 7) << 3)
#define NDTR0_tRP(c)	(min((c), 7) << 0)

#define NDTR1_tR(c)	(min((c), 65535) << 16)
#define NDTR1_tWHR(c)	(min((c), 15) << 4)
#define NDTR1_tAR(c)	(min((c), 15) << 0)

static inline struct mrvl_nand_host *nand_to_host(struct nand_chip *chip)
{
	return container_of(chip, struct mrvl_nand_host, chip);
}

static const struct mrvl_nand_variant pxa3xx_variant = {
	.hwflags	= 0,
};

static const struct mrvl_nand_variant armada370_variant = {
	.hwflags	= HWFLAGS_ECC_BCH | HWFLAGS_HAS_NDCB3,
};

static struct of_device_id mrvl_nand_dt_ids[] = {
	{
		.compatible = "marvell,pxa3xx-nand",
		.data = &pxa3xx_variant,
	},
	{
		.compatible = "marvell,armada370-nand",
		.data = &armada370_variant,
	},
	{}
};
MODULE_DEVICE_TABLE(of, mrvl_nand_dt_ids);

/* convert nano-seconds to nand flash controller clock cycles */
static int ns2cycle(int ns, unsigned long clk_rate)
{
	int clk_mhz = clk_rate / 1000000;

	return roundup(ns * clk_mhz, 1000) / 1000;
}

static volatile u32 _nand_readl(const char *func, const int line,
		       struct mrvl_nand_host *host, int off)
{
	volatile u32 val = readl((host)->mmio_base + (off));

	dev_vdbg(host->dev, "\treadl %s:%d reg=0x%08x => 0x%08x\n",
		func, line, off, val);
	return val;
}

static void _nand_writel(const char *func, const int line,
			 struct mrvl_nand_host *host, int off, u32 val)
{
	dev_vdbg(host->dev, "\twritel %s:%d reg=0x%08x val=0x%08x\n",
		 func, line, off, val);
	writel(val, (host)->mmio_base + off);
}

static struct mrvl_nand_timing timings[] = {
	{ 0x46ec, 10,  0, 20,  40, 30,  40, 11123, 110, 10, },
	{ 0xdaec, 10,  0, 20,  40, 30,  40, 11123, 110, 10, },
	{ 0xd7ec, 10,  0, 20,  40, 30,  40, 11123, 110, 10, },
	{ 0xd3ec,  5, 20, 10,  12, 10,  12, 25000,  60, 10, },
	{ 0xa12c, 10, 25, 15,  25, 15,  30, 25000,  60, 10, },
	{ 0xb12c, 10, 25, 15,  25, 15,  30, 25000,  60, 10, },
	{ 0xdc2c, 10, 25, 15,  25, 15,  30, 25000,  60, 10, },
	{ 0xcc2c, 10, 25, 15,  25, 15,  30, 25000,  60, 10, },
	{ 0xba20, 10, 35, 15,  25, 15,  25, 25000,  60, 10, },
	{ 0x0000, 40, 80, 60, 100, 80, 100, 90000, 400, 40, },
};

static void mrvl_nand_set_timing(struct mrvl_nand_host *host, bool use_default)
{
	struct nand_chip *chip = &host->chip;
	unsigned long nand_clk = clk_get_rate(host->core_clk);
	struct mrvl_nand_timing *t;
	uint32_t ndtr0, ndtr1;
	u16 id;

	if (use_default) {
		id = 0;
	} else {
		chip->legacy.cmdfunc(chip, NAND_CMD_READID, 0x00, -1);
		chip->legacy.read_buf(chip, (unsigned char *)&id, sizeof(id));
	}
	for (t = &timings[0]; t->id; t++)
		if (t->id == id)
			break;
	ndtr0 = NDTR0_tCH(ns2cycle(t->tCH, nand_clk)) |
		NDTR0_tCS(ns2cycle(t->tCS, nand_clk)) |
		NDTR0_tWH(ns2cycle(t->tWH, nand_clk)) |
		NDTR0_tWP(ns2cycle(t->tWP, nand_clk)) |
		NDTR0_tRH(ns2cycle(t->tRH, nand_clk)) |
		NDTR0_tRP(ns2cycle(t->tRP, nand_clk));

	ndtr1 = NDTR1_tR(ns2cycle(t->tR, nand_clk)) |
		NDTR1_tWHR(ns2cycle(t->tWHR, nand_clk)) |
		NDTR1_tAR(ns2cycle(t->tAR, nand_clk));
	nand_writel(host, NDTR0CS0, ndtr0);
	nand_writel(host, NDTR1CS0, ndtr1);
}

static int mrvl_nand_ready(struct nand_chip *chip)
{
	struct mrvl_nand_host *host = nand_to_host(chip);
	u32 ndcr;

	ndcr = nand_readl(host, NDSR);
	if (host->cs == 0)
		return ndcr & NDSR_FLASH_RDY;
	if (host->cs == 1)
		return ndcr & NDSR_RDY;
	return 0;
}

/*
 * Claims all blocks are good.
 *
 * In principle, this function is *only* called when the NAND Flash MTD system
 * isn't allowed to keep an in-memory bad block table, so it is forced to ask
 * the driver for bad block information.
 *
 * In fact, we permit the NAND Flash MTD system to have an in-memory BBT, so
 * this function is *only* called when we take it away.
 *
 * Thus, this function is only called when we want *all* blocks to look good,
 * so it *always* return success.
 */
static int mrvl_nand_block_bad(struct nand_chip *chip, loff_t ofs)
{
	return 0;
}

static void mrvl_nand_select_chip(struct nand_chip *chip, int chipnr)
{
	struct mrvl_nand_host *host = nand_to_host(chip);

	if (chipnr <= 0 || chipnr >= 3 || chipnr == host->cs)
		return;
	host->cs = chipnr - 1;
}

/*
 * Set the data and OOB size, depending on the selected
 * spare and ECC configuration.
 * Only applicable to READ0, READOOB and PAGEPROG commands.
 */
static unsigned int mrvl_datasize(struct mrvl_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);

	unsigned int datasize;

	datasize = mtd->writesize;
	if (host->use_spare) {
		datasize += host->spare_size;
		if (!host->use_ecc)
			datasize += host->ecc_size;
	}
	return datasize;
}

/**
 * NOTE: it is a must to set ND_RUN firstly, then write
 * command buffer, otherwise, it does not work.
 * We enable all the interrupt at the same time, and
 * let mrvl_nand_irq to handle all logic.
 */
static void mrvl_nand_start(struct mrvl_nand_host *host)
{
	uint32_t ndcr;

	if (host->hwflags & HWFLAGS_ECC_BCH) {
		uint32_t reg = nand_readl(host, NDECCCTRL);

		if (host->use_ecc && host->ecc_bch)
			reg |= NDECCCTRL_BCH_EN;
		else
			reg &= ~NDECCCTRL_BCH_EN;

		nand_writel(host, NDECCCTRL, reg);
	}

	ndcr = host->reg_ndcr;
	if (host->use_ecc)
		ndcr |= NDCR_ECC_EN;
	else
		ndcr &= ~NDCR_ECC_EN;

	ndcr &= ~NDCR_DMA_EN;

	if (host->use_spare)
		ndcr |= NDCR_SPARE_EN;
	else
		ndcr &= ~NDCR_SPARE_EN;

	ndcr &= ~NDCR_ND_RUN;
	ndcr |= NDCR_INT_MASK;

	/* clear status bits and run */
	nand_writel(host, NDCR, ndcr);
	nand_writel(host, NDSR, NDSR_MASK);
	nand_writel(host, NDCR, ndcr | NDCR_ND_RUN);

	if (wait_on_timeout(host->chip.legacy.chip_delay * USECOND,
			    nand_readl(host, NDSR) & NDSR_WRCMDREQ)) {
		dev_err(host->dev, "Waiting for command request failed\n");
	} else {
		/*
		 * Command buffer registers NDCB{0-2,3}
		 * must be loaded by writing directly either 12 or 16
		 * bytes directly to NDCB0, four bytes at a time.
		 *
		 * Direct write access to NDCB1, NDCB2 and NDCB3 is ignored
		 * but each NDCBx register can be read.
		 */
		nand_writel(host, NDSR, NDSR_WRCMDREQ);
		nand_writel(host, NDCB0, host->ndcb0);
		nand_writel(host, NDCB0, host->ndcb1);
		nand_writel(host, NDCB0, host->ndcb2);
		if (host->hwflags & HWFLAGS_HAS_NDCB3)
			nand_writel(host, NDCB0, host->ndcb3);
	}
}

static void disable_int(struct mrvl_nand_host *host, uint32_t int_mask)
{
	uint32_t ndcr;

	ndcr = nand_readl(host, NDCR);
	nand_writel(host, NDCR, ndcr | int_mask);
}

static inline int is_buf_blank(uint8_t *buf, size_t len)
{
	for (; len > 0; len--)
		if (*buf++ != 0xff)
			return 0;
	return 1;
}

static void set_command_address(struct mrvl_nand_host *host,
		unsigned int page_size, uint16_t column, int page_addr)
{
	/* small page addr setting */
	if (page_size < PAGE_CHUNK_SIZE) {
		host->ndcb1 = ((page_addr & 0xFFFFFF) << 8)
				| (column & 0xFF);

		host->ndcb2 = 0;
	} else {
		host->ndcb1 = ((page_addr & 0xFFFF) << 16)
				| (column & 0xFFFF);

		if (page_addr & 0xFF0000)
			host->ndcb2 = (page_addr & 0xFF0000) >> 16;
		else
			host->ndcb2 = 0;
	}
}

static void prepare_start_command(struct mrvl_nand_host *host, int command)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);

	/* reset data and oob column point to handle data */
	host->buf_start		= 0;
	host->buf_count		= 0;
	host->oob_size		= 0;
	host->use_ecc		= 0;
	host->use_spare		= 1;
	host->ndcb3		= 0;
	host->cmd_ongoing	= command;

	switch (command) {
	case NAND_CMD_SEQIN:
		/*
		 * This command is a no-op, as merged with PROGPAGE.
		 */
		break;
	case NAND_CMD_READOOB:
		host->data_size = mrvl_datasize(host);
		break;
	case NAND_CMD_READ0:
		host->use_ecc = 1;
		host->data_size = mrvl_datasize(host);
		break;
	case NAND_CMD_PAGEPROG:
		host->use_ecc = 1;
		host->data_size = mrvl_datasize(host);
		break;
	case NAND_CMD_PARAM:
		host->use_spare = 0;
		break;
	default:
		host->ndcb1 = 0;
		host->ndcb2 = 0;
		break;
	}

	/*
	 * If we are about to issue a read command, or about to set
	 * the write address, then clean the data buffer.
	 */
	if (command == NAND_CMD_READ0 ||
	    command == NAND_CMD_READOOB ||
	    command == NAND_CMD_SEQIN) {
		host->buf_count = mtd->writesize + mtd->oobsize;
		memset(host->data_buff, 0xFF, host->buf_count);
	}

}

/**
 * prepare_set_command - Prepare a NAND command
 *
 * Prepare data for a NAND command. If the command will not be executed, but
 * instead merged into a "bi-command", returns 0.
 *
 * Returns if the command should be launched on the NFC
 */
static int prepare_set_command(struct mrvl_nand_host *host, int command,
		int ext_cmd_type, uint16_t column, int page_addr)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	int addr_cycle, exec_cmd;

	exec_cmd = 1;

	if (host->cs != 0)
		host->ndcb0 = NDCB0_CSEL;
	else
		host->ndcb0 = 0;

	addr_cycle = NDCB0_ADDR_CYC(host->row_addr_cycles
				    + host->col_addr_cycles);
	switch (command) {
	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
		host->ndcb0 |= NDCB0_CMD_TYPE(0)
				| addr_cycle
				| NAND_CMD_READ0;

		if (command == NAND_CMD_READOOB)
			host->buf_start = column + mtd->writesize;
		else
			host->buf_start = column;

		/*
		 * Multiple page read needs an 'extended command type' field,
		 * which is either naked-read or last-read according to the
		 * state.
		 */
		if (mtd->writesize == PAGE_CHUNK_SIZE) {
			host->ndcb0 |= NDCB0_DBC | (NAND_CMD_READSTART << 8);
		} else if (mtd->writesize > PAGE_CHUNK_SIZE) {
			host->ndcb0 |= NDCB0_DBC | (NAND_CMD_READSTART << 8)
					| NDCB0_LEN_OVRD
					| NDCB0_EXT_CMD_TYPE(ext_cmd_type);
			host->ndcb3 = host->chunk_size +
				      host->oob_size;
		}

		set_command_address(host, mtd->writesize, column, page_addr);
		break;

	case NAND_CMD_SEQIN:
		host->buf_start = column;
		set_command_address(host, mtd->writesize, 0, page_addr);
		/* Data transfer will occur in write_page */
		host->data_size = 0;
		exec_cmd = 0;
		break;

	case NAND_CMD_PAGEPROG:
		host->ndcb0 |= NDCB0_CMD_TYPE(0x1)
				| NDCB0_AUTO_RS
				| NDCB0_DBC
				| (NAND_CMD_PAGEPROG << 8)
				| NAND_CMD_SEQIN
				| addr_cycle;
		break;

	case NAND_CMD_PARAM:
		host->buf_count = 256;
		host->ndcb0 |= NDCB0_CMD_TYPE(0)
				| NDCB0_ADDR_CYC(1)
				| NDCB0_LEN_OVRD
				| command;
		host->ndcb1 = (column & 0xFF);
		host->ndcb3 = 256;
		host->data_size = 256;
		break;

	case NAND_CMD_READID:
		host->buf_count = host->read_id_bytes;
		host->ndcb0 |= NDCB0_CMD_TYPE(3)
				| NDCB0_ADDR_CYC(1)
				| command;
		host->ndcb1 = (column & 0xFF);

		host->data_size = 8;
		break;
	case NAND_CMD_STATUS:
		host->buf_count = 1;
		host->ndcb0 |= NDCB0_CMD_TYPE(4)
				| NDCB0_ADDR_CYC(1)
				| command;

		host->data_size = 8;
		break;

	case NAND_CMD_ERASE1:
		host->ndcb0 |= NDCB0_CMD_TYPE(2)
				| NDCB0_AUTO_RS
				| NDCB0_ADDR_CYC(3)
				| NDCB0_DBC
				| (NAND_CMD_ERASE2 << 8)
				| NAND_CMD_ERASE1;
		host->ndcb1 = page_addr;
		host->ndcb2 = 0;

		break;
	case NAND_CMD_RESET:
		host->ndcb0 |= NDCB0_CMD_TYPE(5)
				| command;
		break;

	case NAND_CMD_ERASE2:
		exec_cmd = 0;
		break;

	default:
		exec_cmd = 0;
		dev_err(host->dev, "non-supported command %x\n",
				command);
		break;
	}

	return exec_cmd;
}

static void mrvl_data_stage(struct mrvl_nand_host *host)
{
	unsigned int i, mask = NDSR_RDDREQ | NDSR_WRDREQ;
	u32 *src, ndsr;

	dev_dbg(host->dev, "%s() ndsr=0x%08x\n",  __func__,
		nand_readl(host, NDSR));
	if (!host->data_size)
		return;

	wait_on_timeout(host->chip.legacy.chip_delay * USECOND,
			nand_readl(host, NDSR) & mask);
	if (!(nand_readl(host, NDSR) & mask)) {
		dev_err(host->dev, "Timeout waiting for data ndsr=0x%08x\n",
			nand_readl(host, NDSR));
		return;
	}

	ndsr = nand_readl(host, NDSR);
	mask &= ndsr;
	src = (u32 *)host->data_buff;

	for (i = 0; i < host->data_size; i += 4) {
		if (ndsr & NDSR_RDDREQ)
			*src++ = nand_readl(host, NDDB);
		if (ndsr & NDSR_WRDREQ)
			nand_writel(host, NDDB, *src++);
	}

	host->data_size = 0;
	nand_writel(host, NDSR, mask);
}

static void mrvl_nand_wait_cmd_done(struct mrvl_nand_host *host,
				    unsigned command)
{
	unsigned int mask;
	static unsigned int nb_done;

	if (host->cs == 0)
		mask = NDSR_CS0_CMDD;
	else
		mask = NDSR_CS1_CMDD;
	wait_on_timeout(host->chip.legacy.chip_delay * USECOND,
			(nand_readl(host, NDSR) & mask) == mask);
	if ((nand_readl(host, NDSR) & mask) != mask) {
		dev_err(host->dev, "Waiting end of command %dth %d timeout, ndsr=0x%08x ndcr=0x%08x\n",
			nb_done++, command, nand_readl(host, NDSR),
			nand_readl(host, NDCR));
	}
}

static void mrvl_nand_cmdfunc(struct nand_chip *chip, unsigned command,
			       int column, int page_addr)
{
	struct mrvl_nand_host *host = nand_to_host(chip);

	/*
	 * if this is a x16 device ,then convert the input
	 * "byte" address into a "word" address appropriate
	 * for indexing a word-oriented device
	 */
	dev_dbg(host->dev, "%s(cmd=%d, col=%d, page=%d)\n", __func__,
		command, column, page_addr);
	if ((host->reg_ndcr & NDCR_DWIDTH_M) && (command != NAND_CMD_READID))
		column /= 2;

	prepare_start_command(host, command);
	if (prepare_set_command(host, command, 0, column, page_addr)) {
		mrvl_nand_start(host);
		mrvl_data_stage(host);
		mrvl_nand_wait_cmd_done(host, command);
	}
}

/**
 * mrvl_nand_write_page_hwecc - prepare page for write
 *
 * Fills in the host->data_buff. The actual write will be done by the PAGEPROG
 * command, which will trigger a mrvl_data_stage().
 *
 * Returns 0
 */
static int mrvl_nand_write_page_hwecc(struct nand_chip *chip, const uint8_t *buf,
				      int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mrvl_nand_host *host = nand_to_host(chip);

	nand_prog_page_begin_op(chip, page, 0, NULL, 0);

	memcpy(host->data_buff, buf, mtd->writesize);
	if (oob_required)
		memcpy(host->data_buff + mtd->writesize, chip->oob_poi,
		       mtd->oobsize);
	else
		memset(host->data_buff + mtd->writesize, 0xff, mtd->oobsize);
	dev_dbg(host->dev, "%s(buf=%p, oob_required=%d) => 0\n",
		__func__, buf, oob_required);

	return nand_prog_page_end_op(chip);
}

static int mrvl_nand_read_page_hwecc(struct nand_chip *chip, uint8_t *buf,
				     int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mrvl_nand_host *host = nand_to_host(chip);
	u32 ndsr;
	int ret = 0;

	nand_read_page_op(chip, page, 0, NULL, 0);

	chip->legacy.read_buf(chip, buf, mtd->writesize);
	chip->legacy.read_buf(chip, chip->oob_poi, mtd->oobsize);
	ndsr = nand_readl(host, NDSR);

	if (ndsr & NDSR_UNCORERR) {
		if (is_buf_blank(buf, mtd->writesize))
			ret = 0;
		else
			ret = -EBADMSG;
	}
	if (ndsr & NDSR_CORERR) {
		ret = 1;
		if ((host->hwflags & HWFLAGS_ECC_BCH) && host->ecc_bch) {
			ret = NDSR_ERR_CNT(ndsr);
			ndsr &= ~(NDSR_ERR_CNT_MASK << NDSR_ERR_CNT_OFF);
			nand_writel(host, NDSR, ndsr);
		}
	}
	dev_dbg(host->dev, "%s(buf=%p, page=%d, oob_required=%d) => %d\n",
		__func__, buf, page, oob_required, ret);
	return ret;
}

static void mrvl_nand_read_buf(struct nand_chip *chip, uint8_t *buf, int len)
{
	struct mrvl_nand_host *host = nand_to_host(chip);
	int xfer;

	xfer = min_t(int, len, host->buf_count);
	memcpy(buf, host->data_buff + host->buf_start, xfer);
	host->buf_start += xfer;
	host->buf_count -= xfer;
}

static uint8_t mrvl_nand_read_byte(struct nand_chip *chip)
{
	uint8_t ret;

	mrvl_nand_read_buf(chip, (uint8_t *)&ret, sizeof(ret));
	return ret;
}

static u16 mrvl_nand_read_word(struct nand_chip *chip)
{
	u16 ret;

	mrvl_nand_read_buf(chip, (uint8_t *)&ret, sizeof(ret));
	return ret;
}

static void mrvl_nand_write_buf(struct nand_chip *chip,
		const uint8_t *buf, int len)
{
	struct mrvl_nand_host *host = nand_to_host(chip);

	memcpy(host->data_buff + host->buf_start, buf, len);
	host->buf_start += len;
	host->buf_count -= len;
}

static void mrvl_nand_config_flash(struct mrvl_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	uint32_t ndcr = host->reg_ndcr;

	/* calculate flash information */
	host->read_id_bytes = (mtd->writesize == 2048) ? 4 : 2;

	/* calculate addressing information */
	host->col_addr_cycles = (mtd->writesize == 2048) ? 2 : 1;
	if ((mtd->size >> chip->page_shift) > 65536)
		host->row_addr_cycles = 3;
	else
		host->row_addr_cycles = 2;

	ndcr |= NDCR_ND_ARB_EN;
	ndcr |= (host->col_addr_cycles == 2) ? NDCR_RA_START : 0;
	ndcr |= ((mtd->erasesize / mtd->writesize) == 64) ? NDCR_PG_PER_BLK : 0;
	ndcr |= (mtd->writesize == 2048) ? NDCR_PAGE_SZ : 0;

	ndcr &= ~NDCR_RD_ID_CNT_MASK;
	ndcr |= NDCR_RD_ID_CNT(host->read_id_bytes);
	ndcr |= NDCR_SPARE_EN; /* enable spare by default */
	ndcr &= ~NDCR_DMA_EN;

	if (chip->options & NAND_BUSWIDTH_16)
		ndcr |= NDCR_DWIDTH_M | NDCR_DWIDTH_C;
	else
		ndcr &= ~(NDCR_DWIDTH_M | NDCR_DWIDTH_C);

	host->reg_ndcr = ndcr;
}

static int pxa_ecc_strength1(struct mrvl_nand_host *host,
		struct nand_ecc_ctrl *ecc, int ecc_stepsize, int page_size)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);

	if (ecc_stepsize == 512 && page_size == 2048) {
		host->chunk_size = 2048;
		host->spare_size = 40;
		host->ecc_size = 24;
		host->ecc_bch = 0;
		ecc->mode = NAND_ECC_HW;
		ecc->size = 512;
		ecc->strength = 1;
		mtd_set_ecclayout(mtd, &ecc_layout_2KB_hwecc);
		return 0;
	}

	if (ecc_stepsize == 512 && page_size == 512) {
		host->chunk_size = 512;
		host->spare_size = 8;
		host->ecc_size = 8;
		host->ecc_bch = 0;
		ecc->mode = NAND_ECC_HW;
		ecc->size = 512;
		mtd_set_ecclayout(mtd, &ecc_layout_512B_hwecc);
		ecc->strength = 1;
		return 0;
	}

	return -ENODEV;
}

static int pxa_ecc_strength4(struct mrvl_nand_host *host,
		struct nand_ecc_ctrl *ecc, int ecc_stepsize, int page_size)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);

	if (!(host->hwflags & HWFLAGS_ECC_BCH))
		return -ENODEV;

	/*
	 * Required ECC: 4-bit correction per 512 bytes
	 * Select: 16-bit correction per 2048 bytes
	 */
	if (ecc_stepsize == 512 && page_size == 2048) {
		host->chunk_size = 2048;
		host->spare_size = 32;
		host->ecc_size = 32;
		host->ecc_bch = 1;
		ecc->mode = NAND_ECC_HW;
		ecc->size = 2048;
		mtd_set_ecclayout(mtd, &ecc_layout_2KB_bch4bit);
		ecc->strength = 16;
		return 0;
	}

	if (ecc_stepsize == 512 && page_size == 4096) {
		host->chunk_size = 2048;
		host->spare_size = 32;
		host->ecc_size = 32;
		host->ecc_bch = 1;
		ecc->mode = NAND_ECC_HW;
		ecc->size = 2048;
		mtd_set_ecclayout(mtd, &ecc_layout_4KB_bch4bit);
		ecc->strength = 16;
		return 0;
	}

	return -ENODEV;
}

static int pxa_ecc_strength8(struct mrvl_nand_host *host,
		struct nand_ecc_ctrl *ecc, int ecc_stepsize, int page_size)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);

	if (!(host->hwflags & HWFLAGS_ECC_BCH))
		return -ENODEV;

	/*
	 * Required ECC: 8-bit correction per 512 bytes
	 * Select: 16-bit correction per 1024 bytes
	 */
	if (ecc_stepsize == 512 && page_size == 4096) {
		host->chunk_size = 1024;
		host->spare_size = 0;
		host->ecc_size = 32;
		host->ecc_bch = 1;
		ecc->mode = NAND_ECC_HW;
		ecc->size = 1024;
		mtd_set_ecclayout(mtd, &ecc_layout_4KB_bch8bit);
		ecc->strength = 16;
		return 0;
	}

	return -ENODEV;
}

static int pxa_ecc_init(struct mrvl_nand_host *host,
			struct nand_ecc_ctrl *ecc,
			int strength, int ecc_stepsize, int page_size)
{
	int ret;

	switch (strength) {
	case 1:
		ret = pxa_ecc_strength1(host, ecc, ecc_stepsize, page_size);
		break;
	case 4:
		ret = pxa_ecc_strength4(host, ecc, ecc_stepsize, page_size);
		break;
	case 8:
		ret = pxa_ecc_strength8(host, ecc, ecc_stepsize, page_size);
		break;
	default:
		ret = -ENODEV;
		break;
	}

	if (ret) {
		dev_err(host->dev,
			"ECC strength %d at page size %d is not supported\n",
			strength, page_size);
		return ret;
	}

	dev_info(host->dev, "ECC strength %d, ECC step size %d\n",
		 ecc->strength, ecc->size);
	return 0;
}

static int mrvl_nand_scan(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mrvl_nand_host *host = chip->priv;
	int ret;
	unsigned int ndcr;
	uint16_t ecc_strength = host->ecc_strength;
	uint16_t ecc_step = host->ecc_step;

	host->read_id_bytes = 4;
	ndcr = NDCR_ND_ARB_EN | NDCR_SPARE_EN;
	ndcr |= NDCR_RD_ID_CNT(host->read_id_bytes);
	host->reg_ndcr = ndcr;

	/* Device detection must be done with BCH ECC disabled */
	if (host->hwflags & HWFLAGS_ECC_BCH)
		nand_writel(host, NDECCCTRL,
			    nand_readl(host, NDECCCTRL) & ~NDECCCTRL_BCH_EN);

	mrvl_nand_set_timing(host, true);
	if (nand_scan_ident(chip, 1, NULL)) {
		host->reg_ndcr |= NDCR_DWIDTH_M | NDCR_DWIDTH_C;
		if (nand_scan_ident(chip, 1, NULL))
			return -ENODEV;
	}
	mrvl_nand_config_flash(host);
	mrvl_nand_set_timing(host, false);
	if (host->flash_bbt) {
		/*
		 * We'll use a bad block table stored in-flash and don't
		 * allow writing the bad block marker to the flash.
		 */
		chip->bbt_options |=
			NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB_BBM |
			NAND_BBT_CREATE_EMPTY;
		chip->bbt_td = &bbt_main_descr;
		chip->bbt_md = &bbt_mirror_descr;
	}

	/*
	 * If the page size is bigger than the FIFO size, let's check
	 * we are given the right variant and then switch to the extended
	 * (aka split) command handling,
	 */
	if (mtd->writesize > PAGE_CHUNK_SIZE) {
		dev_err(host->dev,
			"unsupported page size on this variant\n");
		return -ENODEV;
	}

	/* Set default ECC strength requirements on non-ONFI devices */
	if (ecc_strength < 1 && ecc_step < 1) {
		ecc_strength = 1;
		ecc_step = 512;
	}

	ret = pxa_ecc_init(host, &chip->ecc, ecc_strength,
			   ecc_step, mtd->writesize);
	if (ret)
		return ret;
	mtd->oobsize = host->spare_size + host->ecc_size;

	/* allocate the real data + oob buffer */
	host->buf_size = mtd->writesize + mtd->oobsize;
	host->data_buff = xmalloc(host->buf_size);

	return nand_scan_tail(chip);
}

static struct mrvl_nand_host *alloc_nand_resource(struct device *dev)
{
	struct resource *iores;
	struct mrvl_nand_platform_data *pdata;
	struct mrvl_nand_host *host;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd;

	pdata = dev->platform_data;
	host = xzalloc(sizeof(*host));
	host->num_cs = 1;
	host->cs = 0;
	mtd = nand_to_mtd(&host->chip);
	mtd->dev.parent = dev;
	mtd->name = "mrvl_nand";

	chip = &host->chip;
	chip->legacy.read_byte = mrvl_nand_read_byte;
	chip->legacy.read_word = mrvl_nand_read_word;
	chip->ecc.read_page	= mrvl_nand_read_page_hwecc;
	chip->ecc.write_page	= mrvl_nand_write_page_hwecc;
	chip->legacy.dev_ready = mrvl_nand_ready;
	chip->legacy.select_chip = mrvl_nand_select_chip;
	chip->legacy.block_bad = mrvl_nand_block_bad;
	chip->legacy.read_buf = mrvl_nand_read_buf;
	chip->legacy.write_buf = mrvl_nand_write_buf;
	chip->options		|= NAND_NO_SUBPAGE_WRITE;
	chip->legacy.cmdfunc = mrvl_nand_cmdfunc;
	chip->priv		= host;
	chip->legacy.chip_delay = CHIP_DELAY_TIMEOUT_US;

	host->dev = dev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return ERR_CAST(iores);
	host->mmio_base = IOMEM(iores->start);
	if (IS_ERR(host->mmio_base)) {
		free(host);
		return ERR_CAST(host->mmio_base);
	}
	host->core_clk = clk_get(dev, NULL);
	if (IS_ERR(host->core_clk)) {
		free(host);
		return ERR_CAST(host->core_clk);
	}
	clk_enable(host->core_clk);

	if (pdata) {
		host->keep_config = pdata->keep_config;
		host->flash_bbt = pdata->flash_bbt;
		host->ecc_strength = pdata->ecc_strength;
		host->ecc_step = pdata->ecc_step_size;
	}

	/* Allocate a buffer to allow flash detection */
	host->buf_size = INIT_BUFFER_SIZE;
	host->data_buff = xmalloc(host->buf_size);

	/* initialize all interrupts to be disabled */
	disable_int(host, NDSR_MASK);
	return host;
}

static int mrvl_nand_probe_dt(struct mrvl_nand_host *host)
{
	struct device_node *np = host->dev->of_node;
	const struct of_device_id *match;
	const struct mrvl_nand_variant *variant;

	if (!IS_ENABLED(CONFIG_OFTREE) || host->dev->platform_data)
		return 0;

	match = of_match_node(mrvl_nand_dt_ids, np);
	if (!match)
		return -EINVAL;
	variant = match->data;

	if (of_get_property(np, "marvell,nand-keep-config", NULL))
		host->keep_config = 1;
	of_property_read_u32(np, "num-cs", &host->num_cs);
	if (of_get_nand_on_flash_bbt(np))
		host->flash_bbt = 1;

	host->ecc_strength = of_get_nand_ecc_strength(np);
	if (host->ecc_strength < 0)
		host->ecc_strength = 0;

	host->ecc_step = of_get_nand_ecc_step_size(np);
	if (host->ecc_step < 0)
		host->ecc_step = 0;

	host->hwflags = variant->hwflags;

	return 0;
}

static int mrvl_nand_probe(struct device *dev)
{
	struct mrvl_nand_host *host;
	struct nand_chip *chip;
	struct mtd_info *mtd;
	int ret;

	host = alloc_nand_resource(dev);
	if (IS_ERR(host)) {
		dev_err(dev, "alloc nand resource failed\n");
		return PTR_ERR(host);
	}

	ret = mrvl_nand_probe_dt(host);
	if (ret)
		return ret;

	chip = &host->chip;
	mtd = nand_to_mtd(chip);

	ret = mrvl_nand_scan(chip);
	if (ret) {
		dev_warn(dev, "failed to scan nand at cs %d\n",
			 host->cs);
		return -ENODEV;
	}

	ret = add_mtd_nand_device(mtd, "nand");
	return ret;
}

static struct driver mrvl_nand_driver = {
	.name		= "mrvl_nand",
	.probe		= mrvl_nand_probe,
	.of_compatible	= DRV_OF_COMPAT(mrvl_nand_dt_ids),
};
device_platform_driver(mrvl_nand_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Marvell NAND controller driver");
