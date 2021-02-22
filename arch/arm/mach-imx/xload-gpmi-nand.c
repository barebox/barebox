/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#define pr_fmt(fmt) "xload-gpmi-nand: " fmt

#include <common.h>
#include <stmp-device.h>
#include <asm-generic/io.h>
#include <linux/sizes.h>
#include <linux/mtd/nand.h>
#include <asm/cache.h>
#include <mach/xload.h>
#include <soc/imx/imx-nand-bcb.h>
#include <linux/mtd/rawnand.h>
#include <mach/imx6-regs.h>
#include <mach/clock-imx6.h>

/*
 * MXS DMA hardware command.
 *
 * This structure describes the in-memory layout of an entire DMA command,
 * including space for the maximum number of PIO accesses. See the appropriate
 * reference manual for a detailed description of what these fields mean to the
 * DMA hardware.
 */
#define	DMACMD_COMMAND_DMA_WRITE	0x1
#define	DMACMD_COMMAND_DMA_READ		0x2
#define	DMACMD_COMMAND_DMA_SENSE	0x3
#define	DMACMD_CHAIN			(1 << 2)
#define	DMACMD_IRQ			(1 << 3)
#define	DMACMD_NAND_LOCK		(1 << 4)
#define	DMACMD_NAND_WAIT_4_READY	(1 << 5)
#define	DMACMD_DEC_SEM			(1 << 6)
#define	DMACMD_WAIT4END			(1 << 7)
#define	DMACMD_HALT_ON_TERMINATE	(1 << 8)
#define	DMACMD_TERMINATE_FLUSH		(1 << 9)
#define	DMACMD_PIO_WORDS(words)		((words) << 12)
#define	DMACMD_XFER_COUNT(x)		((x) << 16)

struct mxs_dma_cmd {
	unsigned long		next;
	unsigned long		data;
	unsigned long		address;
#define	APBH_DMA_PIO_WORDS	6
	unsigned long		pio_words[APBH_DMA_PIO_WORDS];
};

enum mxs_dma_id {
	IMX23_DMA,
	IMX28_DMA,
};

struct apbh_dma {
	void __iomem *regs;
	enum mxs_dma_id id;
};

struct mxs_dma_chan {
	unsigned int flags;
	int channel;
	struct apbh_dma *apbh;
};

#define	HW_APBHX_CTRL0				0x000
#define	BM_APBH_CTRL0_APB_BURST8_EN		(1 << 29)
#define	BM_APBH_CTRL0_APB_BURST_EN		(1 << 28)
#define	BP_APBH_CTRL0_CLKGATE_CHANNEL		8
#define	BP_APBH_CTRL0_RESET_CHANNEL		16
#define	HW_APBHX_CTRL1				0x010
#define	BP_APBHX_CTRL1_CH_CMDCMPLT_IRQ_EN	16
#define	HW_APBHX_CTRL2				0x020
#define	HW_APBHX_CHANNEL_CTRL			0x030
#define	BP_APBHX_CHANNEL_CTRL_RESET_CHANNEL	16
#define	BP_APBHX_VERSION_MAJOR			24
#define	HW_APBHX_CHn_NXTCMDAR_MX23(n)		(0x050 + (n) * 0x70)
#define	HW_APBHX_CHn_NXTCMDAR_MX28(n)		(0x110 + (n) * 0x70)
#define	HW_APBHX_CHn_SEMA_MX23(n)		(0x080 + (n) * 0x70)
#define	HW_APBHX_CHn_SEMA_MX28(n)		(0x140 + (n) * 0x70)
#define	NAND_ONFI_CRC_BASE			0x4f4e

#define apbh_dma_is_imx23(aphb) ((apbh)->id == IMX23_DMA)

/* udelay() is not available in PBL, need to improvise */
static void __udelay(int us)
{
	volatile int i;

	for (i = 0; i < us * 4; i++);
}

/*
 * Enable a DMA channel.
 *
 * If the given channel has any DMA descriptors on its active list, this
 * function causes the DMA hardware to begin processing them.
 *
 * This function marks the DMA channel as "busy," whether or not there are any
 * descriptors to process.
 */
static int mxs_dma_enable(struct mxs_dma_chan *pchan,
	struct mxs_dma_cmd *pdesc)
{
	struct apbh_dma *apbh = pchan->apbh;
	int channel_bit;
	int channel = pchan->channel;

	if (apbh_dma_is_imx23(apbh)) {
		writel((uint32_t)pdesc,
			apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX23(channel));
		writel(1, apbh->regs + HW_APBHX_CHn_SEMA_MX23(channel));
		channel_bit = channel + BP_APBH_CTRL0_CLKGATE_CHANNEL;
	} else {
		writel((uint32_t)pdesc,
			apbh->regs + HW_APBHX_CHn_NXTCMDAR_MX28(channel));
		writel(1, apbh->regs + HW_APBHX_CHn_SEMA_MX28(channel));
		channel_bit = channel;
	}

	writel(1 << channel_bit, apbh->regs +
		HW_APBHX_CTRL0 + STMP_OFFSET_REG_CLR);

	return 0;
}

/*
 * Resets the DMA channel hardware.
 */
static int mxs_dma_reset(struct mxs_dma_chan *pchan)
{
	struct apbh_dma *apbh = pchan->apbh;

	if (apbh_dma_is_imx23(apbh))
		writel(1 << (pchan->channel + BP_APBH_CTRL0_RESET_CHANNEL),
			apbh->regs + HW_APBHX_CTRL0 + STMP_OFFSET_REG_SET);
	else
		writel(1 << (pchan->channel +
			BP_APBHX_CHANNEL_CTRL_RESET_CHANNEL),
			apbh->regs + HW_APBHX_CHANNEL_CTRL +
			STMP_OFFSET_REG_SET);

	return 0;
}

static int mxs_dma_wait_complete(struct mxs_dma_chan *pchan)
{
	struct apbh_dma *apbh = pchan->apbh;
	int timeout = 1000000;

	while (1) {
		if (readl(apbh->regs + HW_APBHX_CTRL1) & (1 << pchan->channel))
			return 0;

		if (!timeout--)
			return -ETIMEDOUT;
	}
}

/*
 * Execute the DMA channel
 */
static int mxs_dma_run(struct mxs_dma_chan *pchan, struct mxs_dma_cmd *pdesc,
		int num)
{
	struct apbh_dma *apbh = pchan->apbh;
	int i, ret;

	/* chain descriptors */
	for (i = 0; i < num - 1; i++) {
		pdesc[i].next = (uint32_t)(&pdesc[i + 1]);
		pdesc[i].data |= DMACMD_CHAIN;
	}

	writel(1 << (pchan->channel + BP_APBHX_CTRL1_CH_CMDCMPLT_IRQ_EN),
		apbh->regs + HW_APBHX_CTRL1 + STMP_OFFSET_REG_SET);

	ret = mxs_dma_enable(pchan, pdesc);
	if (ret) {
		pr_err("%s: Failed to enable dma channel: %d\n",
			__func__, ret);
		return ret;
	}

	ret = mxs_dma_wait_complete(pchan);
	if (ret) {
		pr_err("%s: Failed to wait for completion: %d\n",
			__func__, ret);
		return ret;
	}

	/* Shut the DMA channel down. */
	writel(1 << pchan->channel, apbh->regs +
		HW_APBHX_CTRL1 + STMP_OFFSET_REG_CLR);
	writel(1 << pchan->channel, apbh->regs +
		HW_APBHX_CTRL2 + STMP_OFFSET_REG_CLR);

	mxs_dma_reset(pchan);

	writel(1 << (pchan->channel + BP_APBHX_CTRL1_CH_CMDCMPLT_IRQ_EN),
		apbh->regs + HW_APBHX_CTRL1 + STMP_OFFSET_REG_CLR);

	return 0;
}

/* ----------------------------- NAND driver part -------------------------- */

#define	GPMI_CTRL0					0x00000000
#define	GPMI_CTRL0_RUN					(1 << 29)
#define	GPMI_CTRL0_DEV_IRQ_EN				(1 << 28)
#define	GPMI_CTRL0_UDMA					(1 << 26)
#define	GPMI_CTRL0_COMMAND_MODE_MASK			(0x3 << 24)
#define	GPMI_CTRL0_COMMAND_MODE_OFFSET			24
#define	GPMI_CTRL0_COMMAND_MODE_WRITE			(0x0 << 24)
#define	GPMI_CTRL0_COMMAND_MODE_READ			(0x1 << 24)
#define	GPMI_CTRL0_COMMAND_MODE_READ_AND_COMPARE	(0x2 << 24)
#define	GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY		(0x3 << 24)
#define	GPMI_CTRL0_WORD_LENGTH				(1 << 23)
#define	GPMI_CTRL0_CS(cs)				((cs) << 20)
#define	GPMI_CTRL0_ADDRESS_MASK				(0x7 << 17)
#define	GPMI_CTRL0_ADDRESS_OFFSET			17
#define	GPMI_CTRL0_ADDRESS_NAND_DATA			(0x0 << 17)
#define	GPMI_CTRL0_ADDRESS_NAND_CLE			(0x1 << 17)
#define	GPMI_CTRL0_ADDRESS_NAND_ALE			(0x2 << 17)
#define	GPMI_CTRL0_ADDRESS_INCREMENT			(1 << 16)
#define	GPMI_CTRL0_XFER_COUNT_MASK			0xffff
#define	GPMI_CTRL0_XFER_COUNT_OFFSET			0

#define	GPMI_ECCCTRL_ECC_CMD_DECODE			(0x0 << 13)
#define	GPMI_ECCCTRL_ENABLE_ECC				(1 << 12)
#define	GPMI_ECCCTRL_BUFFER_MASK_BCH_PAGE		0x1ff

#define	BCH_CTRL					0x00000000
#define	BCH_CTRL_COMPLETE_IRQ				(1 << 0)

#define	MXS_NAND_DMA_DESCRIPTOR_COUNT			6
#define	MXS_NAND_CHUNK_DATA_CHUNK_SIZE			512
#define	MXS_NAND_METADATA_SIZE				10
#define	MXS_NAND_COMMAND_BUFFER_SIZE			128

struct mxs_nand_info {
	void __iomem *io_base;
	void __iomem *bch_base;
	struct mxs_dma_chan *dma_channel;
	int cs;
	uint8_t *cmd_buf;
	struct mxs_dma_cmd *desc;
	struct fcb_block fcb;
	int dbbt_num_entries;
	u32 *dbbt;
	struct nand_memory_organization organization;
	unsigned long nand_size;
};

/**
 * It was discovered that xloading barebox from NAND sometimes fails. Observed
 * behaviour is similar to silicon errata ERR007117 for i.MX6.
 *
 * ERR007117 description:
 * For raw NAND boot, ROM switches the source of enfc_clk_root from PLL2_PFD2
 * to PLL3. The root clock is required to be gated before switching the source
 * clock. If the root clock is not gated, clock glitches might be passed to the
 * divider that follows the clock mux, and the divider might behave
 * unpredictably. This can cause the clock generation to fail and the chip will
 * not boot successfully.
 *
 * Workaround solution for this errata:
 * 1) gate all GPMI/BCH related clocks (CG15, G14, CG13, CG12 and CG6)
 * 2) reconfigure clocks
 * 3) ungate all GPMI/BCH related clocks
 *
 */
static inline void imx6_errata_007117_enable(void)
{
	u32 reg;

	/* Gate (disable) the GPMI/BCH clocks in CCM_CCGR4 */
	reg = readl(MXC_CCM_CCGR4);
	reg &= ~(0xFF003000);
	writel(reg, MXC_CCM_CCGR4);

	/**
	 * Gate (disable) the enfc_clk_root before changing the enfc_clk_root
	 * source or dividers by clearing CCM_CCGR2[CG7] to 2'b00. This
	 * disables the iomux_ipt_clk_io_clk.
	 */
	reg = readl(MXC_CCM_CCGR2);
	reg &= ~(0x3 << 14);
	writel(reg, MXC_CCM_CCGR2);

	/* Configure CCM_CS2CDR for the new clock source configuration */
	reg = readl(MXC_CCM_CS2CDR);
	reg &= ~(0x7FF0000);
	writel(reg, MXC_CCM_CS2CDR);
	reg |= 0xF0000;
	writel(reg, MXC_CCM_CS2CDR);

	/**
	 * Enable enfc_clk_root by setting CCM_CCGR2[CG7] to 2'b11. This
	 * enables the iomux_ipt_clk_io_clk.
	 */
	reg = readl(MXC_CCM_CCGR2);
	reg |= 0x3 << 14;
	writel(reg, MXC_CCM_CCGR2);

	/* Ungate (enable) the GPMI/BCH clocks in CCM_CCGR4 */
	reg = readl(MXC_CCM_CCGR4);
	reg |= 0xFF003000;
	writel(reg, MXC_CCM_CCGR4);
}

static uint32_t mxs_nand_aux_status_offset(void)
{
	return (MXS_NAND_METADATA_SIZE + 0x3) & ~0x3;
}

static int mxs_nand_read_page(struct mxs_nand_info *info, int writesize,
		int oobsize, int pagenum, void *databuf, int raw)
{
	void __iomem *bch_regs = info->bch_base;
	unsigned column = 0;
	struct mxs_dma_cmd *d;
	int cmd_queue_len;
	u8 *cmd_buf;
	int ret;
	uint8_t	*status;
	int i;
	int timeout;
	int descnum = 0;
	int max_pagenum = info->nand_size /
		info->organization.pagesize;

	memset(info->desc, 0,
		sizeof(*info->desc) * MXS_NAND_DMA_DESCRIPTOR_COUNT);

	/* Compile DMA descriptor - read0 */
	cmd_buf = info->cmd_buf;
	cmd_queue_len = 0;
	d = &info->desc[descnum++];
	d->address = (dma_addr_t)(cmd_buf);
	cmd_buf[cmd_queue_len++] = NAND_CMD_READ0;
	cmd_buf[cmd_queue_len++] = column;
	cmd_buf[cmd_queue_len++] = column >> 8;
	cmd_buf[cmd_queue_len++] = pagenum;
	cmd_buf[cmd_queue_len++] = pagenum >> 8;

	if ((max_pagenum - 1) >= SZ_64K)
		cmd_buf[cmd_queue_len++] = pagenum >> 16;

	d->data = DMACMD_COMMAND_DMA_READ |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(cmd_queue_len);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		cmd_queue_len;

	/* Compile DMA descriptor - readstart */
	cmd_buf = &info->cmd_buf[8];
	cmd_queue_len = 0;
	d = &info->desc[descnum++];
	d->address = (dma_addr_t)(cmd_buf);

	cmd_buf[cmd_queue_len++] = NAND_CMD_READSTART;

	d->data = DMACMD_COMMAND_DMA_READ |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(cmd_queue_len);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		cmd_queue_len;

	/* Compile DMA descriptor - wait for ready. */
	d = &info->desc[descnum++];
	d->data = DMACMD_CHAIN |
		DMACMD_NAND_WAIT_4_READY |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(2);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_DATA;

	if (raw) {
		/* Compile DMA descriptor - read. */
		d = &info->desc[descnum++];
		d->data = DMACMD_WAIT4END |
			DMACMD_PIO_WORDS(1) |
			DMACMD_XFER_COUNT(writesize + oobsize) |
			DMACMD_COMMAND_DMA_WRITE;
		d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_READ |
			GPMI_CTRL0_WORD_LENGTH |
			GPMI_CTRL0_CS(info->cs) |
			GPMI_CTRL0_ADDRESS_NAND_DATA |
			(writesize + oobsize);
		d->address = (dma_addr_t)databuf;
	} else {
		/* Compile DMA descriptor - enable the BCH block and read. */
		d = &info->desc[descnum++];
		d->data = DMACMD_WAIT4END | DMACMD_PIO_WORDS(6);
		d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_READ |
			GPMI_CTRL0_WORD_LENGTH |
			GPMI_CTRL0_CS(info->cs) |
			GPMI_CTRL0_ADDRESS_NAND_DATA |
			(writesize + oobsize);
		d->pio_words[1] = 0;
		d->pio_words[2] = GPMI_ECCCTRL_ENABLE_ECC |
			GPMI_ECCCTRL_ECC_CMD_DECODE |
			GPMI_ECCCTRL_BUFFER_MASK_BCH_PAGE;
		d->pio_words[3] = writesize + oobsize;
		d->pio_words[4] = (dma_addr_t)databuf;
		d->pio_words[5] = (dma_addr_t)(databuf + writesize);

		/* Compile DMA descriptor - disable the BCH block. */
		d = &info->desc[descnum++];
		d->data = DMACMD_NAND_WAIT_4_READY |
			DMACMD_WAIT4END |
			DMACMD_PIO_WORDS(3);
		d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
			GPMI_CTRL0_WORD_LENGTH |
			GPMI_CTRL0_CS(info->cs) |
			GPMI_CTRL0_ADDRESS_NAND_DATA |
			(writesize + oobsize);
	}

	/* Compile DMA descriptor - de-assert the NAND lock and interrupt. */
	d = &info->desc[descnum++];
	d->data = DMACMD_IRQ | DMACMD_DEC_SEM;

	/* Execute the DMA chain. */
	ret = mxs_dma_run(info->dma_channel, info->desc, descnum);
	if (ret) {
		pr_err("DMA read error\n");
		goto err;
	}

	if (raw)
		return 0;

	timeout = 1000000;

	while (1) {
		if (!timeout--) {
			ret = -ETIMEDOUT;
			goto err;
		}

		if (readl(bch_regs + BCH_CTRL) & BCH_CTRL_COMPLETE_IRQ)
			break;
	}

	writel(BCH_CTRL_COMPLETE_IRQ,
		bch_regs + BCH_CTRL + STMP_OFFSET_REG_CLR);

	/* Loop over status bytes, accumulating ECC status. */
	status = databuf + writesize + mxs_nand_aux_status_offset();
	for (i = 0; i < writesize / MXS_NAND_CHUNK_DATA_CHUNK_SIZE; i++) {
		if (status[i] == 0xfe) {
			ret = -EBADMSG;
			goto err;
		}
	}

	ret = 0;
err:
	return ret;
}

static int mxs_nand_get_read_status(struct mxs_nand_info *info, void *databuf)
{
	int ret;
	u8 *cmd_buf;
	struct mxs_dma_cmd *d;
	int descnum = 0;
	int cmd_queue_len;

	memset(info->desc, 0,
		sizeof(*info->desc) * MXS_NAND_DMA_DESCRIPTOR_COUNT);

	/* Compile DMA descriptor - READ STATUS */
	cmd_buf = info->cmd_buf;
	cmd_queue_len = 0;
	d = &info->desc[descnum++];
	d->address = (dma_addr_t)(cmd_buf);
	cmd_buf[cmd_queue_len++] = NAND_CMD_STATUS;

	d->data = DMACMD_COMMAND_DMA_READ |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(cmd_queue_len);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		cmd_queue_len;

	/* Compile DMA descriptor - read. */
	d = &info->desc[descnum++];
	d->data = DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(1) |
		DMACMD_COMMAND_DMA_WRITE;
	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		(1);
	d->address = (dma_addr_t)databuf;

	/* Compile DMA descriptor - de-assert the NAND lock and interrupt. */
	d = &info->desc[descnum++];
	d->data = DMACMD_IRQ | DMACMD_DEC_SEM;

	/* Execute the DMA chain. */
	ret = mxs_dma_run(info->dma_channel, info->desc, descnum);
	if (ret) {
		pr_err("DMA read error\n");
		return ret;
	}

	return ret;
}

static int mxs_nand_reset(struct mxs_nand_info *info, void *databuf)
{
	int ret, i;
	u8 *cmd_buf;
	struct mxs_dma_cmd *d;
	int descnum = 0;
	int cmd_queue_len;
	u8 read_status;

	memset(info->desc, 0,
		sizeof(*info->desc) * MXS_NAND_DMA_DESCRIPTOR_COUNT);

	/* Compile DMA descriptor - RESET */
	cmd_buf = info->cmd_buf;
	cmd_queue_len = 0;
	d = &info->desc[descnum++];
	d->address = (dma_addr_t)(cmd_buf);
	cmd_buf[cmd_queue_len++] = NAND_CMD_RESET;

	d->data = DMACMD_COMMAND_DMA_READ |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(cmd_queue_len);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		cmd_queue_len;

	/* Compile DMA descriptor - de-assert the NAND lock and interrupt. */
	d = &info->desc[descnum++];
	d->data = DMACMD_IRQ | DMACMD_DEC_SEM;

	/* Execute the DMA chain. */
	ret = mxs_dma_run(info->dma_channel, info->desc, descnum);
	if (ret) {
		pr_err("DMA read error\n");
		return ret;
	}

	/* Wait for NAND to wake up */
	for (i = 0; i < 10; i++) {
		__udelay(50000);
		ret = mxs_nand_get_read_status(info, databuf);
		if (ret)
			return ret;
		memcpy(&read_status, databuf, 1);
		if (read_status & NAND_STATUS_READY)
			return 0;
	}

	pr_warn("NAND Reset failed\n");
	return -1;
}

/* function taken from nand_onfi.c */
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

static int mxs_nand_get_onfi(struct mxs_nand_info *info, void *databuf)
{
	int ret;
	u8 *cmd_buf;
	u16 crc;
	struct mxs_dma_cmd *d;
	int descnum = 0;
	int cmd_queue_len;
	struct nand_onfi_params nand_params;

	memset(info->desc, 0,
		sizeof(*info->desc) * MXS_NAND_DMA_DESCRIPTOR_COUNT);

	/* Compile DMA descriptor - READ PARAMETER PAGE */
	cmd_buf = info->cmd_buf;
	cmd_queue_len = 0;
	d = &info->desc[descnum++];
	d->address = (dma_addr_t)(cmd_buf);
	cmd_buf[cmd_queue_len++] = NAND_CMD_PARAM;
	cmd_buf[cmd_queue_len++] = 0x00;

	d->data = DMACMD_COMMAND_DMA_READ |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(cmd_queue_len);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		cmd_queue_len;

	/* Compile DMA descriptor - wait for ready. */
	d = &info->desc[descnum++];
	d->data = DMACMD_CHAIN |
		DMACMD_NAND_WAIT_4_READY |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(2);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_DATA;

	/* Compile DMA descriptor - read. */
	d = &info->desc[descnum++];
	d->data = DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(sizeof(struct nand_onfi_params)) |
		DMACMD_COMMAND_DMA_WRITE;
	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		(sizeof(struct nand_onfi_params));
	d->address = (dma_addr_t)databuf;

	/* Compile DMA descriptor - de-assert the NAND lock and interrupt. */
	d = &info->desc[descnum++];
	d->data = DMACMD_IRQ | DMACMD_DEC_SEM;

	/* Execute the DMA chain. */
	ret = mxs_dma_run(info->dma_channel, info->desc, descnum);
	if (ret) {
		pr_err("DMA read error\n");
		return ret;
	}

	memcpy(&nand_params, databuf, sizeof(struct nand_onfi_params));

	crc = onfi_crc16(NAND_ONFI_CRC_BASE, (u8 *)&nand_params, 254);
	pr_debug("ONFI CRC: 0x%x, CALC. CRC 0x%x\n", nand_params.crc, crc);
	if (crc != le16_to_cpu(nand_params.crc)) {
		pr_debug("ONFI CRC mismatch!\n");
		ret = -EUCLEAN;
		return ret;
	}

	/* Fill the NAND organization struct with data */
	info->organization.bits_per_cell = nand_params.bits_per_cell;
	info->organization.pagesize = le32_to_cpu(nand_params.byte_per_page);
	info->organization.oobsize =
		le16_to_cpu(nand_params.spare_bytes_per_page);
	info->organization.pages_per_eraseblock =
		le32_to_cpu(nand_params.pages_per_block);
	info->organization.eraseblocks_per_lun =
		le32_to_cpu(nand_params.blocks_per_lun);
	info->organization.max_bad_eraseblocks_per_lun =
		le16_to_cpu(nand_params.bb_per_lun);
	info->organization.luns_per_target = nand_params.lun_count;
	info->nand_size = info->organization.pagesize *
		info->organization.pages_per_eraseblock *
		info->organization.eraseblocks_per_lun *
		info->organization.luns_per_target;

	return ret;
}

static int mxs_nand_check_onfi(struct mxs_nand_info *info, void *databuf)
{
	int ret;
	u8 *cmd_buf;
	struct mxs_dma_cmd *d;
	int descnum = 0;
	int cmd_queue_len;

	struct onfi_header {
		u8 byte0;
		u8 byte1;
		u8 byte2;
		u8 byte3;
	} onfi_head;

	memset(info->desc, 0,
		sizeof(*info->desc) * MXS_NAND_DMA_DESCRIPTOR_COUNT);

	/* Compile DMA descriptor - READID + 0x20 (ADDR) */
	cmd_buf = info->cmd_buf;
	cmd_queue_len = 0;
	d = &info->desc[descnum++];
	d->address = (dma_addr_t)(cmd_buf);
	cmd_buf[cmd_queue_len++] = NAND_CMD_READID;
	cmd_buf[cmd_queue_len++] = 0x20;

	d->data = DMACMD_COMMAND_DMA_READ |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(cmd_queue_len);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		cmd_queue_len;

	/* Compile DMA descriptor - read. */
	d = &info->desc[descnum++];
	d->data = DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(sizeof(struct onfi_header)) |
		DMACMD_COMMAND_DMA_WRITE;
	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		(sizeof(struct onfi_header));
	d->address = (dma_addr_t)databuf;

	/* Compile DMA descriptor - de-assert the NAND lock and interrupt. */
	d = &info->desc[descnum++];
	d->data = DMACMD_IRQ | DMACMD_DEC_SEM;

	/* Execute the DMA chain. */
	ret = mxs_dma_run(info->dma_channel, info->desc, descnum);
	if (ret) {
		pr_err("DMA read error\n");
		return ret;
	}

	memcpy(&onfi_head, databuf, sizeof(struct onfi_header));

	pr_debug("ONFI Byte0: 0x%x\n", onfi_head.byte0);
	pr_debug("ONFI Byte1: 0x%x\n", onfi_head.byte1);
	pr_debug("ONFI Byte2: 0x%x\n", onfi_head.byte2);
	pr_debug("ONFI Byte3: 0x%x\n", onfi_head.byte3);

	/* check if returned values correspond to ascii characters "ONFI" */
	if (onfi_head.byte0 != 0x4f || onfi_head.byte1 != 0x4e ||
		onfi_head.byte2 != 0x46 || onfi_head.byte3 != 0x49)
		return 1;

	return 0;
}

static int mxs_nand_get_readid(struct mxs_nand_info *info, void *databuf)
{
	int ret;
	u8 *cmd_buf;
	struct mxs_dma_cmd *d;
	int descnum = 0;
	int cmd_queue_len;

	struct readid_data {
		u8 byte0;
		u8 byte1;
		u8 byte2;
		u8 byte3;
		u8 byte4;
	} id_data;

	memset(info->desc, 0,
		sizeof(*info->desc) * MXS_NAND_DMA_DESCRIPTOR_COUNT);

	/* Compile DMA descriptor - READID */
	cmd_buf = info->cmd_buf;
	cmd_queue_len = 0;
	d = &info->desc[descnum++];
	d->address = (dma_addr_t)(cmd_buf);
	cmd_buf[cmd_queue_len++] = NAND_CMD_READID;
	cmd_buf[cmd_queue_len++] = 0x00;

	d->data = DMACMD_COMMAND_DMA_READ |
		DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(cmd_queue_len);

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		cmd_queue_len;

	/* Compile DMA descriptor - read. */
	d = &info->desc[descnum++];
	d->data = DMACMD_WAIT4END |
		DMACMD_PIO_WORDS(1) |
		DMACMD_XFER_COUNT(sizeof(struct readid_data)) |
		DMACMD_COMMAND_DMA_WRITE;
	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		GPMI_CTRL0_CS(info->cs) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		(sizeof(struct readid_data));
	d->address = (dma_addr_t)databuf;

	/* Compile DMA descriptor - de-assert the NAND lock and interrupt. */
	d = &info->desc[descnum++];
	d->data = DMACMD_IRQ | DMACMD_DEC_SEM;

	/* Execute the DMA chain. */
	ret = mxs_dma_run(info->dma_channel, info->desc, descnum);
	if (ret) {
		pr_err("DMA read error\n");
		return ret;
	}

	memcpy(&id_data, databuf, sizeof(struct readid_data));

	pr_debug("NAND Byte0: 0x%x\n", id_data.byte0);
	pr_debug("NAND Byte1: 0x%x\n", id_data.byte1);
	pr_debug("NAND Byte2: 0x%x\n", id_data.byte2);
	pr_debug("NAND Byte3: 0x%x\n", id_data.byte3);
	pr_debug("NAND Byte4: 0x%x\n", id_data.byte4);

	if (id_data.byte0 == 0xff || id_data.byte1 == 0xff ||
		id_data.byte2 == 0xff || id_data.byte3 == 0xff ||
		id_data.byte4 == 0xff) {
		pr_err("\"READ ID\" returned 0xff, possible error!\n");
		return -EOVERFLOW;
	}

	/* Fill the NAND organization struct with data */
	info->organization.bits_per_cell =
		(1 << ((id_data.byte2 >> 2) & 0x3)) * 2;
	info->organization.pagesize =
		(1 << (id_data.byte3 & 0x3)) * SZ_1K;
	info->organization.oobsize = id_data.byte3 & 0x4 ?
		info->organization.pagesize / 512 * 16 :
		info->organization.pagesize / 512 * 8;
	info->organization.pages_per_eraseblock =
		(1 << ((id_data.byte3 >> 4) & 0x3)) * SZ_64K /
		info->organization.pagesize;
	info->organization.planes_per_lun =
		1 << ((id_data.byte4 >> 2) & 0x3);
	info->nand_size = info->organization.planes_per_lun *
		(1 << ((id_data.byte4 >> 4) & 0x7)) * SZ_8M;
	info->organization.eraseblocks_per_lun = info->nand_size /
		(info->organization.pages_per_eraseblock *
		info->organization.pagesize);

	return ret;
}

static int mxs_nand_get_info(struct mxs_nand_info *info, void *databuf)
{
	int ret, i;

	ret = mxs_nand_check_onfi(info, databuf);
	if (ret) {
		if (ret != 1)
			return ret;
		pr_info("ONFI not supported, try \"READ ID\"...\n");
	} else {
		/*
		 * Some NAND's don't return the correct data the first time
		 * "READ PARAMETER PAGE" is returned. Execute the command
		 * multimple times
		 */
		for (i = 0; i < 3; i++) {
			/*
			 * Some NAND's need to be reset before "READ PARAMETER
			 * PAGE" can be successfully executed.
			 */
			ret = mxs_nand_reset(info, databuf);
			if (ret)
				return ret;
			ret = mxs_nand_get_onfi(info, databuf);
			if (ret)
				pr_err("ONFI error: %d\n", ret);
			else
				break;
		}
		if (!ret)
			goto succ;
	}

	/*
	 * If ONFI is not supported or if it fails try to get NAND's info from
	 * "READ ID" command.
	 */
	ret = mxs_nand_reset(info, databuf);
	if (ret)
		return ret;
	pr_debug("Trying \"READ ID\" command...\n");
	ret = mxs_nand_get_readid(info, databuf);
	if (ret) {
		pr_err("xloader supports only ONFI and generic \"READ ID\" " \
			"supported NANDs\n");
		return -1;
	}
succ:
	pr_debug("NAND page_size: %d\n", info->organization.pagesize);
	pr_debug("NAND block_size: %d\n",
		info->organization.pages_per_eraseblock
		* info->organization.pagesize);
	pr_debug("NAND oob_size: %d\n", info->organization.oobsize);
	pr_debug("NAND nand_size: %lu\n", info->nand_size);
	pr_debug("NAND bits_per_cell: %d\n", info->organization.bits_per_cell);
	pr_debug("NAND planes_per_lun: %d\n",
		info->organization.planes_per_lun);
	pr_debug("NAND luns_per_target: %d\n",
		info->organization.luns_per_target);
	pr_debug("NAND eraseblocks_per_lun: %d\n",
		info->organization.eraseblocks_per_lun);
	pr_debug("NAND ntargets: %d\n", info->organization.ntargets);


	return 0;
}

/* ---------------------------- BCB handling part -------------------------- */

static uint32_t calc_chksum(void *buf, size_t size)
{
	u32 chksum = 0;
	u8 *bp = buf;
	size_t i;

	for (i = 0; i < size; i++)
		chksum += bp[i];

	return ~chksum;
}

static int get_fcb(struct mxs_nand_info *info, void *databuf)
{
	int i, pagenum, ret;
	uint32_t checksum;
	struct fcb_block *fcb = &info->fcb;

	/* First page read fails, this shouldn't be necessary */
	mxs_nand_read_page(info, info->organization.pagesize,
		info->organization.oobsize, 0, databuf, 1);

	for (i = 0; i < 4; i++) {
		pagenum = info->organization.pages_per_eraseblock * i;

		ret = mxs_nand_read_page(info, info->organization.pagesize,
			info->organization.oobsize, pagenum, databuf, 1);
		if (ret)
			continue;

		memcpy(fcb, databuf + mxs_nand_aux_status_offset(),
			sizeof(*fcb));

		if (fcb->FingerPrint != FCB_FINGERPRINT) {
			pr_err("No FCB found on page %d\n", pagenum);
			continue;
		}

		checksum = calc_chksum((void *)fcb +
			sizeof(uint32_t), sizeof(*fcb) - sizeof(uint32_t));

		if (checksum != fcb->Checksum) {
			pr_err("FCB on page %d has invalid checksum. " \
				"Expected: 0x%08x, calculated: 0x%08x",
				pagenum, fcb->Checksum, checksum);
			continue;
		}

		pr_debug("Found FCB:\n");
		pr_debug("PageDataSize:     0x%08x\n", fcb->PageDataSize);
		pr_debug("TotalPageSize:    0x%08x\n", fcb->TotalPageSize);
		pr_debug("SectorsPerBlock:  0x%08x\n", fcb->SectorsPerBlock);
		pr_debug("FW1_startingPage: 0x%08x\n",
			fcb->Firmware1_startingPage);
		pr_debug("PagesInFW1:       0x%08x\n", fcb->PagesInFirmware1);
		pr_debug("FW2_startingPage: 0x%08x\n",
			fcb->Firmware2_startingPage);
		pr_debug("PagesInFW2:       0x%08x\n", fcb->PagesInFirmware2);

		return 0;
	}

	return -EINVAL;
}

static int get_dbbt(struct mxs_nand_info *info, void *databuf)
{
	int i, ret;
	int page;
	int startpage = info->fcb.DBBTSearchAreaStartAddress;
	struct dbbt_block dbbt;

	for (i = 0; i < 4; i++) {
		page = startpage + i * info->organization.pages_per_eraseblock;

		ret = mxs_nand_read_page(info, info->organization.pagesize,
			info->organization.oobsize, page, databuf, 0);
		if (ret)
			continue;

		memcpy(&dbbt, databuf, sizeof(struct dbbt_block));

		if (*(u32 *)(databuf + sizeof(u32)) != DBBT_FINGERPRINT)
			continue;

		/* Version check */
		if (be32_to_cpup(databuf + 2 * sizeof(u32)) < 1)
			return -ENOENT;

		ret = mxs_nand_read_page(info, info->organization.pagesize,
			info->organization.oobsize, page + 4, databuf, 0);
		if (ret)
			continue;

		info->dbbt_num_entries = *(u32 *)(databuf + sizeof(u32));

		pr_debug("Found DBBT with %d entries\n",
			info->dbbt_num_entries);
		pr_debug("Checksum = 0x%08x\n", dbbt.Checksum);
		pr_debug("FingerPrint = 0x%08x\n", dbbt.FingerPrint);
		pr_debug("Version = 0x%08x\n", dbbt.Version);
		pr_debug("numberBB = 0x%08x\n", dbbt.numberBB);
		pr_debug("DBBTNumOfPages= 0x%08x\n", dbbt.DBBTNumOfPages);

		for (i = 0; i < info->dbbt_num_entries; i++)
			pr_debug("badblock %d at block %d\n", i,
				*(u32 *)(databuf + (2 + i) * sizeof(u32)));

		info->dbbt = databuf + 2 * sizeof(u32);

		return 0;
	}

	return -ENOENT;
}

static int block_is_bad(struct mxs_nand_info *info, int blocknum)
{
	int i;
	u32 *dbbt = info->dbbt;

	if (!dbbt)
		return 0;

	for (i = 0; i < info->dbbt_num_entries; i++) {
		if (dbbt[i] == blocknum)
			return 1;
	}

	return 0;
}

static int read_firmware(struct mxs_nand_info *info, int startpage,
	void *dest, int len)
{
	int curpage = startpage;
	struct fcb_block *fcb = &info->fcb;
	int pagesperblock = fcb->SectorsPerBlock;
	int numpages = (len / fcb->PageDataSize) + 1;
	int ret;
	int pagesize = fcb->PageDataSize;
	int oobsize = fcb->TotalPageSize - pagesize;

	pr_debug("Reading %d pages starting from page %d\n",
		numpages, startpage);

	if (block_is_bad(info, curpage / pagesperblock))
		curpage = ALIGN_DOWN(curpage + pagesperblock, pagesperblock);

	while (numpages) {
		if (!(curpage & (pagesperblock - 1))) {
			/* Check for bad blocks on each block boundary */
			if (block_is_bad(info, curpage / pagesperblock)) {
				pr_debug("Skipping bad block at page %d\n",
					curpage);
				curpage += pagesperblock;
				continue;
			}
		}

		ret = mxs_nand_read_page(info, pagesize, oobsize,
			curpage, dest, 0);
		if (ret) {
			pr_debug("Failed to read page %d\n", curpage);
			return ret;
		}

		*((u8 *)dest + fcb->BadBlockMarkerByte) =
			*(u8 *)(dest + pagesize);

		numpages--;
		dest += pagesize;
		curpage++;
	}

	return 0;
}

static int __maybe_unused imx6_nand_load_image(void *cmdbuf, void *descs,
	void *databuf, void *dest, int len)
{
	struct mxs_nand_info info = {
		.io_base = (void *)0x00112000,
		.bch_base = (void *)0x00114000,
	};
	struct apbh_dma apbh = {
		.id = IMX28_DMA,
		.regs = (void *)0x00110000,
	};
	struct mxs_dma_chan pchan = {
		.channel = 0, /* MXS: MXS_DMA_CHANNEL_AHB_APBH_GPMI0 */
		.apbh = &apbh,
	};
	int ret;
	struct fcb_block *fcb;

	info.dma_channel = &pchan;

	pr_debug("cmdbuf: 0x%p descs: 0x%p databuf: 0x%p dest: 0x%p\n",
			cmdbuf, descs, databuf, dest);

	/* Command buffers */
	info.cmd_buf = cmdbuf;
	info.desc = descs;

	ret = mxs_nand_get_info(&info, databuf);
	if (ret)
		return ret;

	ret = get_fcb(&info, databuf);
	if (ret)
		return ret;

	fcb = &info.fcb;

	get_dbbt(&info, databuf);

	ret = read_firmware(&info, fcb->Firmware1_startingPage, dest, len);
	if (ret) {
		pr_err("Failed to read firmware1, trying firmware2\n");
		ret = read_firmware(&info, fcb->Firmware2_startingPage,
			dest, len);
		if (ret) {
			pr_err("Failed to also read firmware2\n");
			return ret;
		}
	}

	return 0;
}

int imx6_nand_start_image(void)
{
	int ret;
	void *sdram = (void *)0x10000000;
	void __noreturn (*bb)(void);
	void *cmdbuf, *databuf, *descs;

	cmdbuf = sdram;
	descs = sdram + MXS_NAND_COMMAND_BUFFER_SIZE;
	databuf = descs +
		sizeof(struct mxs_dma_cmd) * MXS_NAND_DMA_DESCRIPTOR_COUNT;
	bb = (void *)PAGE_ALIGN((unsigned long)databuf + SZ_8K);

	/* Apply ERR007117 workaround */
	imx6_errata_007117_enable();

	ret = imx6_nand_load_image(cmdbuf, descs, databuf,
		bb, imx_image_size());
	if (ret) {
		pr_err("Loading image failed: %d\n", ret);
		return ret;
	}

	pr_debug("Starting barebox image at 0x%p\n", bb);

	arm_early_mmu_cache_invalidate();
	barrier();

	bb();
}
