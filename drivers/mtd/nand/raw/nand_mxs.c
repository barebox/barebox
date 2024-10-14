// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Freescale i.MX28 NAND flash driver
 *
 * Copyright (C) 2011 Wolfram Sang <w.sang@pengutronix.de>
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * Based on code from LTIB:
 * Freescale GPMI NFC NAND Flash Driver
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 * Copyright (C) 2008 Embedded Alley Solutions, Inc.
 */

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/nand_mxs.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/bitfield.h>
#include <of_mtd.h>
#include <common.h>
#include <dma.h>
#include <malloc.h>
#include <errno.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <dma/apbh-dma.h>
#include <stmp-device.h>
#include <mach/imx/generic.h>
#include <soc/imx/gpmi-nand.h>

#include "internals.h"

enum gpmi_type {
	GPMI_MXS,
	GPMI_IMX6,
};

/**
 * struct nand_timing - Fundamental timing attributes for NAND.
 * @data_setup_in_ns:         The data setup time, in nanoseconds. Usually the
 *                            maximum of tDS and tWP. A negative value
 *                            indicates this characteristic isn't known.
 * @data_hold_in_ns:          The data hold time, in nanoseconds. Usually the
 *                            maximum of tDH, tWH and tREH. A negative value
 *                            indicates this characteristic isn't known.
 * @address_setup_in_ns:      The address setup time, in nanoseconds. Usually
 *                            the maximum of tCLS, tCS and tALS. A negative
 *                            value indicates this characteristic isn't known.
 * @gpmi_sample_delay_in_ns:  A GPMI-specific timing parameter. A negative value
 *                            indicates this characteristic isn't known.
 * @tREA_in_ns:               tREA, in nanoseconds, from the data sheet. A
 *                            negative value indicates this characteristic isn't
 *                            known.
 * @tRLOH_in_ns:              tRLOH, in nanoseconds, from the data sheet. A
 *                            negative value indicates this characteristic isn't
 *                            known.
 * @tRHOH_in_ns:              tRHOH, in nanoseconds, from the data sheet. A
 *                            negative value indicates this characteristic isn't
 *                            known.
 */
struct nand_timing {
	int8_t  data_setup_in_ns;
	int8_t  data_hold_in_ns;
	int8_t  address_setup_in_ns;
	int8_t  gpmi_sample_delay_in_ns;
	int8_t  tREA_in_ns;
	int8_t  tRLOH_in_ns;
	int8_t  tRHOH_in_ns;
};

struct mxs_nand_info {
	struct device		*dev;
	struct nand_chip	nand_chip;
	void __iomem		*io_base;
	void __iomem		*bch_base;
	struct clk		*clk;
	enum gpmi_type		type;
	int			dma_channel_base;
	u32		version;

	int		cur_chip;

	uint32_t	cmd_queue_len;

	uint8_t		*cmd_buf;
	uint8_t		*data_buf;
	uint8_t		*oob_buf;

	uint8_t		raw_oob_mode;

	/* Functions with altered behaviour */
	int		(*hooked_read_oob)(struct mtd_info *mtd,
				loff_t from, struct mtd_oob_ops *ops);
	int		(*hooked_write_oob)(struct mtd_info *mtd,
				loff_t to, struct mtd_oob_ops *ops);

	/* DMA descriptors */
	struct mxs_dma_cmd	*desc;
	uint32_t		desc_index;

#define GPMI_ASYNC_EDO_ENABLED	(1 << 0)
#define GPMI_TIMING_INIT_OK	(1 << 1)
	unsigned		flags;
	struct nand_timing	timing;

	int		bb_mark_bit_offset;
};

static inline int mxs_nand_is_imx6(struct mxs_nand_info *info)
{
	return info->type == GPMI_IMX6;
}

static struct mxs_dma_cmd *mxs_nand_get_dma_desc(struct mxs_nand_info *info)
{
	struct mxs_dma_cmd *desc;

	if (info->desc_index >= MXS_NAND_DMA_DESCRIPTOR_COUNT) {
		printf("MXS NAND: Too many DMA descriptors requested\n");
		return NULL;
	}

	desc = &info->desc[info->desc_index];
	info->desc_index++;

	return desc;
}

static void mxs_nand_return_dma_descs(struct mxs_nand_info *info)
{
	int i;
	struct mxs_dma_cmd *desc;

	for (i = 0; i < info->desc_index; i++) {
		desc = &info->desc[i];
		memset(desc, 0, sizeof(struct mxs_dma_cmd));
	}

	info->desc_index = 0;
}

/*
 * We don't support writing the oob area so simply return the whole oob
 * as ECC.
 */
static int mxs_nand_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *oobregion)
{
	if (section)
		return -ERANGE;

	oobregion->offset = 0;
	oobregion->length = mtd->oobsize;

	return 0;
}

static int mxs_nand_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *oobregion)
{
	return -ERANGE;
}

static const struct mtd_ooblayout_ops mxs_nand_ooblayout_ops = {
	.ecc = mxs_nand_ooblayout_ecc,
	.free = mxs_nand_ooblayout_free,
};

static uint32_t mxs_nand_ecc_chunk_cnt(uint32_t page_data_size)
{
	return page_data_size / MXS_NAND_CHUNK_DATA_CHUNK_SIZE;
}

static uint32_t mxs_nand_aux_status_offset(void)
{
	return (MXS_NAND_METADATA_SIZE + 0x3) & ~0x3;
}

static uint32_t mxs_nand_get_mark_offset(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	uint32_t chunk_data_size_in_bits;
	uint32_t chunk_ecc_size_in_bits;
	uint32_t chunk_total_size_in_bits;
	uint32_t block_mark_chunk_number;
	uint32_t block_mark_chunk_bit_offset;
	uint32_t block_mark_bit_offset;

	chunk_data_size_in_bits = MXS_NAND_CHUNK_DATA_CHUNK_SIZE * 8;
	chunk_ecc_size_in_bits  = chip->ecc.strength * 13;

	chunk_total_size_in_bits =
			chunk_data_size_in_bits + chunk_ecc_size_in_bits;

	/* Compute the bit offset of the block mark within the physical page. */
	block_mark_bit_offset = mtd->writesize * 8;

	/* Subtract the metadata bits. */
	block_mark_bit_offset -= MXS_NAND_METADATA_SIZE * 8;

	/*
	 * Compute the chunk number (starting at zero) in which the block mark
	 * appears.
	 */
	block_mark_chunk_number =
			block_mark_bit_offset / chunk_total_size_in_bits;

	/*
	 * Compute the bit offset of the block mark within its chunk, and
	 * validate it.
	 */
	block_mark_chunk_bit_offset = block_mark_bit_offset -
			(block_mark_chunk_number * chunk_total_size_in_bits);

	if (block_mark_chunk_bit_offset > chunk_data_size_in_bits)
		return 1;

	/*
	 * Now that we know the chunk number in which the block mark appears,
	 * we can subtract all the ECC bits that appear before it.
	 */
	block_mark_bit_offset -=
		block_mark_chunk_number * chunk_ecc_size_in_bits;

	return block_mark_bit_offset;
}

static int mxs_nand_calc_geo(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mxs_nand_info *nand_info = chip->priv;
	int ecc_chunk_count = mxs_nand_ecc_chunk_cnt(mtd->writesize);
	int gf_len = 13;  /* length of Galois Field for non-DDR nand */
	int max_ecc_strength;

	max_ecc_strength = ((mtd->oobsize - MXS_NAND_METADATA_SIZE) * 8)
			   / (gf_len * ecc_chunk_count);
	/* We need the minor even number. */
	max_ecc_strength = rounddown(max_ecc_strength, 2);

	if (chip->ecc.strength) {
		if (chip->ecc.strength > max_ecc_strength) {
			dev_err(nand_info->dev, "invalid ecc strength %d (maximum %d)\n",
				chip->ecc.strength, max_ecc_strength);
			return -EINVAL;
		}
	} else {
		chip->ecc.strength = max_ecc_strength;
	}

	if (chip->ecc.size && chip->ecc.size != MXS_NAND_CHUNK_DATA_CHUNK_SIZE) {
		dev_err(nand_info->dev, "invalid ecc size %d, this driver only supports %d\n",
			chip->ecc.size, MXS_NAND_CHUNK_DATA_CHUNK_SIZE);
		return -EINVAL;
	}

	chip->ecc.bytes = DIV_ROUND_UP(13 * chip->ecc.strength, 8);
	chip->ecc.size = MXS_NAND_CHUNK_DATA_CHUNK_SIZE;

	nand_info->bb_mark_bit_offset = mxs_nand_get_mark_offset(mtd);

	return 0;
}

static struct mtd_info *mxs_nand_mtd;

int mxs_nand_get_geo(int *ecc_strength, int *bb_mark_bit_offset)
{
	struct nand_chip *chip;
	struct mxs_nand_info *nand_info;

	if (!mxs_nand_mtd)
		return -ENODEV;

	chip = mtd_to_nand(mxs_nand_mtd);
	nand_info = chip->priv;

	*ecc_strength = chip->ecc.strength;
	*bb_mark_bit_offset = nand_info->bb_mark_bit_offset;

	return 0;
}

/*
 * Wait for BCH complete IRQ and clear the IRQ
 */
static int mxs_nand_wait_for_bch_complete(struct mxs_nand_info *nand_info)
{
	void __iomem *bch_regs = nand_info->bch_base;
	int timeout = MXS_NAND_BCH_TIMEOUT;
	int ret;

	while (--timeout) {
		if (readl(bch_regs + BCH_CTRL) & BCH_CTRL_COMPLETE_IRQ)
			break;
		udelay(1);
	}

	ret = (timeout == 0) ? -ETIMEDOUT : 0;

	writel(BCH_CTRL_COMPLETE_IRQ, bch_regs + BCH_CTRL + STMP_OFFSET_REG_CLR);

	return ret;
}

/*
 * This is the function that we install in the cmd_ctrl function pointer of the
 * owning struct nand_chip. The only functions in the reference implementation
 * that use these functions pointers are cmdfunc and select_chip.
 *
 * In this driver, we implement our own select_chip, so this function will only
 * be called by the reference implementation's cmdfunc. For this reason, we can
 * ignore the chip enable bit and concentrate only on sending bytes to the NAND
 * Flash.
 */
static void mxs_nand_cmd_ctrl(struct nand_chip *chip, int data, unsigned int ctrl)
{
	struct mxs_nand_info *nand_info = chip->priv;
	struct mxs_dma_cmd *d;
	uint32_t channel = nand_info->dma_channel_base + nand_info->cur_chip;
	int ret;

	/*
	 * If this condition is true, something is _VERY_ wrong in MTD
	 * subsystem!
	 */
	if (nand_info->cmd_queue_len == MXS_NAND_COMMAND_BUFFER_SIZE) {
		printf("MXS NAND: Command queue too long\n");
		return;
	}

	/*
	 * Every operation begins with a command byte and a series of zero or
	 * more address bytes. These are distinguished by either the Address
	 * Latch Enable (ALE) or Command Latch Enable (CLE) signals being
	 * asserted. When MTD is ready to execute the command, it will
	 * deasert both latch enables.
	 *
	 * Rather than run a separate DMA operation for every single byte, we
	 * queue them up and run a single DMA operation for the entire series
	 * of command and data bytes.
	 */
	if (ctrl & (NAND_ALE | NAND_CLE)) {
		if (data != NAND_CMD_NONE)
			nand_info->cmd_buf[nand_info->cmd_queue_len++] = data;
		return;
	}

	/*
	 * If control arrives here, MTD has deasserted both the ALE and CLE,
	 * which means it's ready to run an operation. Check if we have any
	 * bytes to send.
	 */
	if (nand_info->cmd_queue_len == 0)
		return;

	/* Compile the DMA descriptor -- a descriptor that sends command. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_DMA_READ | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_CHAIN | MXS_DMA_DESC_DEC_SEM |
		MXS_DMA_DESC_WAIT4END | MXS_DMA_DESC_PIO_WORDS(3) |
		MXS_DMA_DESC_XFER_COUNT(nand_info->cmd_queue_len);

	d->address = (dma_addr_t)nand_info->cmd_buf;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		nand_info->cmd_queue_len;

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel, nand_info->desc, nand_info->desc_index);
	if (ret)
		printf("MXS NAND: Error sending command (%d)\n", ret);

	mxs_nand_return_dma_descs(nand_info);

	/* Reset the command queue. */
	nand_info->cmd_queue_len = 0;
}

/*
 * Test if the NAND flash is ready.
 */
static int mxs_nand_device_ready(struct nand_chip *chip)
{
	struct mxs_nand_info *nand_info = chip->priv;
	void __iomem *gpmi_regs = nand_info->io_base;
	uint32_t tmp;

	if (nand_info->version > GPMI_VERSION_TYPE_MX23) {
		tmp = readl(gpmi_regs + GPMI_STAT);
		/* i.MX6 has only one R/B actual pin, so if there are several
		   R/B signals they must be all connected to this pin */
		if (cpu_is_mx6())
			tmp >>= GPMI_STAT_READY_BUSY_OFFSET;
		else
			tmp >>= (GPMI_STAT_READY_BUSY_OFFSET +
					 nand_info->cur_chip);
	} else {
		tmp = readl(gpmi_regs + GPMI_DEBUG);
		tmp >>= (GPMI_DEBUG_READY0_OFFSET + nand_info->cur_chip);
	}
	return tmp & 1;
}

/*
 * Select the NAND chip.
 */
static void mxs_nand_select_chip(struct nand_chip *chip, int chipnum)
{
	struct mxs_nand_info *nand_info = chip->priv;

	nand_info->cur_chip = chipnum;
}

/*
 * Handle block mark swapping.
 *
 * Note that, when this function is called, it doesn't know whether it's
 * swapping the block mark, or swapping it *back* -- but it doesn't matter
 * because the the operation is the same.
 */
static void mxs_nand_swap_block_mark(struct nand_chip *chip,
					uint8_t *data_buf, uint8_t *oob_buf)
{
	struct mxs_nand_info *nand_info = chip->priv;

	uint32_t bit_offset;
	uint32_t buf_offset;

	uint32_t src;
	uint32_t dst;

	/* Don't do swapping on MX23 */
	if (nand_info->version == GPMI_VERSION_TYPE_MX23)
		return;

	bit_offset = nand_info->bb_mark_bit_offset & 0x7;
	buf_offset = nand_info->bb_mark_bit_offset >> 3;

	/*
	 * Get the byte from the data area that overlays the block mark. Since
	 * the ECC engine applies its own view to the bits in the page, the
	 * physical block mark won't (in general) appear on a byte boundary in
	 * the data.
	 */
	src = data_buf[buf_offset] >> bit_offset;
	src |= data_buf[buf_offset + 1] << (8 - bit_offset);

	dst = oob_buf[0];

	oob_buf[0] = src;

	data_buf[buf_offset] &= ~(0xff << bit_offset);
	data_buf[buf_offset + 1] &= 0xff << bit_offset;

	data_buf[buf_offset] |= dst << bit_offset;
	data_buf[buf_offset + 1] |= dst >> (8 - bit_offset);
}

/*
 * Read data from NAND.
 */
static void mxs_nand_read_buf(struct nand_chip *chip, uint8_t *buf, int length)
{
	struct mxs_nand_info *nand_info = chip->priv;
	struct mxs_dma_cmd *d;
	uint32_t channel = nand_info->dma_channel_base + nand_info->cur_chip;
	int ret;

	if (length > NAND_MAX_PAGESIZE) {
		printf("MXS NAND: DMA buffer too big\n");
		return;
	}

	if (!buf) {
		printf("MXS NAND: DMA buffer is NULL\n");
		return;
	}

	/* Compile the DMA descriptor - a descriptor that reads data. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_DMA_WRITE | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_DEC_SEM | MXS_DMA_DESC_WAIT4END |
		MXS_DMA_DESC_PIO_WORDS(1) |
		MXS_DMA_DESC_XFER_COUNT(length);

	d->address = (dma_addr_t)nand_info->data_buf;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		length;

	/*
	 * A DMA descriptor that waits for the command to end and the chip to
	 * become ready.
	 *
	 * I think we actually should *not* be waiting for the chip to become
	 * ready because, after all, we don't care. I think the original code
	 * did that and no one has re-thought it yet.
	 */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_NAND_WAIT_4_READY | MXS_DMA_DESC_DEC_SEM |
		MXS_DMA_DESC_WAIT4END | MXS_DMA_DESC_PIO_WORDS(4);

	d->address = 0;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_DATA;

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel, nand_info->desc, nand_info->desc_index);
	if (ret) {
		printf("MXS NAND: DMA read error\n");
		goto rtn;
	}

	memcpy(buf, nand_info->data_buf, length);

rtn:
	mxs_nand_return_dma_descs(nand_info);
}

/*
 * Write data to NAND.
 */
static void mxs_nand_write_buf(struct nand_chip *chip, const uint8_t *buf,
				int length)
{
	struct mxs_nand_info *nand_info = chip->priv;
	struct mxs_dma_cmd *d;
	uint32_t channel = nand_info->dma_channel_base + nand_info->cur_chip;
	int ret;

	if (length > NAND_MAX_PAGESIZE) {
		printf("MXS NAND: DMA buffer too big\n");
		return;
	}

	if (!buf) {
		printf("MXS NAND: DMA buffer is NULL\n");
		return;
	}

	memcpy(nand_info->data_buf, buf, length);

	/* Compile the DMA descriptor - a descriptor that writes data. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_DMA_READ | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_DEC_SEM | MXS_DMA_DESC_WAIT4END |
		MXS_DMA_DESC_PIO_WORDS(4) |
		MXS_DMA_DESC_XFER_COUNT(length);

	d->address = (dma_addr_t)nand_info->data_buf;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		length;

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel, nand_info->desc, nand_info->desc_index);
	if (ret)
		printf("MXS NAND: DMA write error\n");

	mxs_nand_return_dma_descs(nand_info);
}

/*
 * Read a single byte from NAND.
 */
static uint8_t mxs_nand_read_byte(struct nand_chip *chip)
{
	uint8_t buf;
	mxs_nand_read_buf(chip, &buf, 1);
	return buf;
}

static void mxs_nand_config_bch(struct nand_chip *chip, int readlen)
{
	struct mxs_nand_info *nand_info = chip->priv;
	int chunk_size;
	void __iomem *bch_regs = nand_info->bch_base;
	u32 fl0, fl1;

	if (mxs_nand_is_imx6(nand_info))
		chunk_size = MXS_NAND_CHUNK_DATA_CHUNK_SIZE >> 2;
	else
		chunk_size = MXS_NAND_CHUNK_DATA_CHUNK_SIZE;

	fl0 = FIELD_PREP(BCH_FLASHLAYOUT0_NBLOCKS, mxs_nand_ecc_chunk_cnt(readlen) - 1);
	fl0 |= FIELD_PREP(BCH_FLASHLAYOUT0_META_SIZE, MXS_NAND_METADATA_SIZE);
	if (mxs_nand_is_imx6(nand_info))
		fl0 |= FIELD_PREP(IMX6_BCH_FLASHLAYOUT0_ECC0, chip->ecc.strength >> 1);
	else
		fl0 |= FIELD_PREP(BCH_FLASHLAYOUT0_ECC0, chip->ecc.strength >> 1);
	fl0 |= FIELD_PREP(BCH_FLASHLAYOUT0_DATA0_SIZE, chunk_size);
	writel(fl0, bch_regs + BCH_FLASH0LAYOUT0);

	fl1 = FIELD_PREP(BCH_FLASHLAYOUT1_PAGE_SIZE, readlen);
	if (mxs_nand_is_imx6(nand_info))
		fl1 |= FIELD_PREP(IMX6_BCH_FLASHLAYOUT1_ECCN, chip->ecc.strength >> 1);
	else
		fl1 |= FIELD_PREP(BCH_FLASHLAYOUT1_ECCN, chip->ecc.strength >> 1);

	fl1 |= FIELD_PREP(BCH_FLASHLAYOUT1_DATAN_SIZE, chunk_size);
	writel(fl1, bch_regs + BCH_FLASH0LAYOUT1);
}

static int mxs_nand_do_bch_read(struct nand_chip *chip, int channel, int readtotal,
				bool randomizer, int page)
{
	struct mxs_nand_info *nand_info = chip->priv;
	struct mxs_dma_cmd *d;
	int ret;

	/* Compile the DMA descriptor - wait for ready. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
		MXS_DMA_DESC_NAND_WAIT_4_READY | MXS_DMA_DESC_WAIT4END |
		MXS_DMA_DESC_PIO_WORDS(1);

	d->address = 0;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_DATA;

	/* Compile the DMA descriptor - enable the BCH block and read. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
		MXS_DMA_DESC_WAIT4END |	MXS_DMA_DESC_PIO_WORDS(6);

	d->address = 0;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		readtotal;
	d->pio_words[1] = 0;
	d->pio_words[2] =
		GPMI_ECCCTRL_ENABLE_ECC |
		GPMI_ECCCTRL_ECC_CMD_DECODE |
		GPMI_ECCCTRL_BUFFER_MASK_BCH_PAGE;
	d->pio_words[3] = readtotal;
	d->pio_words[4] = (dma_addr_t)nand_info->data_buf;
	d->pio_words[5] = (dma_addr_t)nand_info->oob_buf;

	if (randomizer) {
		d->pio_words[2] |= GPMI_ECCCTRL_RANDOMIZER_ENABLE |
				       GPMI_ECCCTRL_RANDOMIZER_TYPE2;
		d->pio_words[3] |= (page % 256) << 16;
	}

	/* Compile the DMA descriptor - disable the BCH block. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
		MXS_DMA_DESC_NAND_WAIT_4_READY | MXS_DMA_DESC_WAIT4END |
		MXS_DMA_DESC_PIO_WORDS(3);

	d->address = 0;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		readtotal;
	d->pio_words[1] = 0;
	d->pio_words[2] = 0;

	/* Compile the DMA descriptor - deassert the NAND lock and interrupt. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_DEC_SEM;

	d->address = 0;

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel, nand_info->desc, nand_info->desc_index);
	if (ret) {
		dev_err(nand_info->dev, "MXS NAND: DMA read error (ecc)\n");
		goto out;
	}

	ret = mxs_nand_wait_for_bch_complete(nand_info);
	if (ret) {
		dev_err(nand_info->dev, "MXS NAND: BCH read timeout\n");
		goto out;
	}

out:
	mxs_nand_return_dma_descs(nand_info);

	return ret;
}

/*
 * Read a page from NAND.
 */
static int __mxs_nand_ecc_read_page(struct nand_chip *chip,
					uint8_t *buf, int oob_required, int page,
					int readlen)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mxs_nand_info *nand_info = chip->priv;
	uint32_t channel = nand_info->dma_channel_base + nand_info->cur_chip;
	uint32_t corrected = 0, failed = 0;
	uint8_t	*status;
	unsigned int  max_bitflips = 0;
	int i, ret, readtotal, nchunks;

	nand_read_page_op(chip, page, 0, NULL, 0);

	readlen = roundup(readlen, MXS_NAND_CHUNK_DATA_CHUNK_SIZE);
	nchunks = mxs_nand_ecc_chunk_cnt(readlen);
	readtotal =  MXS_NAND_METADATA_SIZE;
	readtotal += MXS_NAND_CHUNK_DATA_CHUNK_SIZE * nchunks;
	readtotal += DIV_ROUND_UP(13 * chip->ecc.strength * nchunks, 8);

	mxs_nand_config_bch(chip, readtotal);

	mxs_nand_do_bch_read(chip, channel, readtotal, false, page);

	/* Read DMA completed, now do the mark swapping. */
	mxs_nand_swap_block_mark(chip, nand_info->data_buf, nand_info->oob_buf);

	memcpy(buf, nand_info->data_buf, readlen);

	/* Loop over status bytes, accumulating ECC status. */
	status = nand_info->oob_buf + mxs_nand_aux_status_offset();
	for (i = 0; i < nchunks; i++) {
		if (status[i] == 0x00)
			continue;

		if (status[i] == 0xff)
			continue;

		if (status[i] == 0xfe) {
			int flips;

			/*
			 * The ECC hardware has an uncorrectable ECC status
			 * code in case we have bitflips in an erased page. As
			 * nothing was written into this subpage the ECC is
			 * obviously wrong and we can not trust it. We assume
			 * at this point that we are reading an erased page and
			 * try to correct the bitflips in buffer up to
			 * ecc_strength bitflips. If this is a page with random
			 * data, we exceed this number of bitflips and have a
			 * ECC failure. Otherwise we use the corrected buffer.
			 */
			if (i == 0) {
				/* The first block includes metadata */
				flips = nand_check_erased_ecc_chunk(
						buf + i * MXS_NAND_CHUNK_DATA_CHUNK_SIZE,
						MXS_NAND_CHUNK_DATA_CHUNK_SIZE,
						NULL, 0,
						nand_info->oob_buf,
						MXS_NAND_METADATA_SIZE,
						mtd->ecc_strength);
			} else {
				flips = nand_check_erased_ecc_chunk(
						buf + i * MXS_NAND_CHUNK_DATA_CHUNK_SIZE,
						MXS_NAND_CHUNK_DATA_CHUNK_SIZE,
						NULL, 0,
						NULL, 0,
						mtd->ecc_strength);
			}

			if (flips > 0) {
				max_bitflips = max_t(unsigned int,
						     max_bitflips, flips);
				corrected += flips;
				continue;
			}

			failed++;
			continue;
		}

		corrected += status[i];
		max_bitflips = max_t(unsigned int, max_bitflips, status[i]);
	}

	/* Propagate ECC status to the owning MTD. */
	mtd->ecc_stats.failed += failed;
	mtd->ecc_stats.corrected += corrected;

	/*
	 * It's time to deliver the OOB bytes. See mxs_nand_ecc_read_oob() for
	 * details about our policy for delivering the OOB.
	 *
	 * We fill the caller's buffer with set bits, and then copy the block
	 * mark to the caller's buffer. Note that, if block mark swapping was
	 * necessary, it has already been done, so we can rely on the first
	 * byte of the auxiliary buffer to contain the block mark.
	 */
	memset(chip->oob_poi, 0xff, mtd->oobsize);

	chip->oob_poi[0] = nand_info->oob_buf[0];

	ret = 0;

	mxs_nand_return_dma_descs(nand_info);

	mxs_nand_config_bch(chip, mtd->writesize + mtd->oobsize);

	return ret ? ret : max_bitflips;
}

static int mxs_nand_ecc_read_page(struct nand_chip *chip,
					uint8_t *buf, int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);

	return __mxs_nand_ecc_read_page(chip, buf, oob_required, page,
					mtd->writesize);
}

static int gpmi_ecc_read_subpage(struct nand_chip *chip,
			uint32_t offs, uint32_t len, uint8_t *buf, int page)
{
	/*
	 * For now always read from the beginning of a page. Allowing
	 * offsets here makes __mxs_nand_ecc_read_page() more
	 * complicated.
	 */
	if (offs) {
		len += offs;
		offs = 0;
	}

	return __mxs_nand_ecc_read_page(chip, buf, 0, page, len);
}

/*
 * Write a page to NAND.
 */
static int mxs_nand_ecc_write_page(struct nand_chip *chip, const uint8_t *buf,
				int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mxs_nand_info *nand_info = chip->priv;
	struct mxs_dma_cmd *d;
	uint32_t channel = nand_info->dma_channel_base + nand_info->cur_chip;
	int ret = 0;

	nand_prog_page_begin_op(chip, page, 0, NULL, 0);

	memcpy(nand_info->data_buf, buf, mtd->writesize);
	memcpy(nand_info->oob_buf, chip->oob_poi, mtd->oobsize);

	/* Handle block mark swapping. */
	mxs_nand_swap_block_mark(chip, nand_info->data_buf, nand_info->oob_buf);

	/* Compile the DMA descriptor - write data. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_DEC_SEM | MXS_DMA_DESC_WAIT4END |
		MXS_DMA_DESC_PIO_WORDS(6);

	d->address = 0;

	d->pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		FIELD_PREP(GPMI_CTRL0_CS, nand_info->cur_chip) |
		GPMI_CTRL0_ADDRESS_NAND_DATA;
	d->pio_words[1] = 0;
	d->pio_words[2] =
		GPMI_ECCCTRL_ENABLE_ECC |
		GPMI_ECCCTRL_ECC_CMD_ENCODE |
		GPMI_ECCCTRL_BUFFER_MASK_BCH_PAGE;
	d->pio_words[3] = (mtd->writesize + mtd->oobsize);
	d->pio_words[4] = (dma_addr_t)nand_info->data_buf;
	d->pio_words[5] = (dma_addr_t)nand_info->oob_buf;

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel, nand_info->desc, nand_info->desc_index);
	if (ret) {
		printf("MXS NAND: DMA write error\n");
		goto rtn;
	}

	ret = mxs_nand_wait_for_bch_complete(nand_info);
	if (ret) {
		printf("MXS NAND: BCH write timeout\n");
		goto rtn;
	}

rtn:
	mxs_nand_return_dma_descs(nand_info);

	if (ret)
		return ret;

	return nand_prog_page_end_op(chip);
}

/*
 * Read OOB from NAND.
 *
 * This function is a veneer that replaces the function originally installed by
 * the NAND Flash MTD code.
 */
static int mxs_nand_hook_read_oob(struct mtd_info *mtd, loff_t from,
					struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mxs_nand_info *nand_info = chip->priv;
	int ret;

	if (ops->mode == MTD_OPS_RAW)
		nand_info->raw_oob_mode = 1;
	else
		nand_info->raw_oob_mode = 0;

	ret = nand_info->hooked_read_oob(mtd, from, ops);

	nand_info->raw_oob_mode = 0;

	return ret;
}

/*
 * Write OOB to NAND.
 *
 * This function is a veneer that replaces the function originally installed by
 * the NAND Flash MTD code.
 */
static int mxs_nand_hook_write_oob(struct mtd_info *mtd, loff_t to,
					struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct mxs_nand_info *nand_info = chip->priv;
	int ret;

	if (ops->mode == MTD_OPS_RAW)
		nand_info->raw_oob_mode = 1;
	else
		nand_info->raw_oob_mode = 0;

	ret = nand_info->hooked_write_oob(mtd, to, ops);

	nand_info->raw_oob_mode = 0;

	return ret;
}

/*
 * There are several places in this driver where we have to handle the OOB and
 * block marks. This is the function where things are the most complicated, so
 * this is where we try to explain it all. All the other places refer back to
 * here.
 *
 * These are the rules, in order of decreasing importance:
 *
 * 1) Nothing the caller does can be allowed to imperil the block mark, so all
 *    write operations take measures to protect it.
 *
 * 2) In read operations, the first byte of the OOB we return must reflect the
 *    true state of the block mark, no matter where that block mark appears in
 *    the physical page.
 *
 * 3) ECC-based read operations return an OOB full of set bits (since we never
 *    allow ECC-based writes to the OOB, it doesn't matter what ECC-based reads
 *    return).
 *
 * 4) "Raw" read operations return a direct view of the physical bytes in the
 *    page, using the conventional definition of which bytes are data and which
 *    are OOB. This gives the caller a way to see the actual, physical bytes
 *    in the page, without the distortions applied by our ECC engine.
 *
 * What we do for this specific read operation depends on whether we're doing
 * "raw" read, or an ECC-based read.
 *
 * It turns out that knowing whether we want an "ECC-based" or "raw" read is not
 * easy. When reading a page, for example, the NAND Flash MTD code calls our
 * ecc.read_page or ecc.read_page_raw function. Thus, the fact that MTD wants an
 * ECC-based or raw view of the page is implicit in which function it calls
 * (there is a similar pair of ECC-based/raw functions for writing).
 *
 * Since MTD assumes the OOB is not covered by ECC, there is no pair of
 * ECC-based/raw functions for reading or or writing the OOB. The fact that the
 * caller wants an ECC-based or raw view of the page is not propagated down to
 * this driver.
 *
 * Since our OOB *is* covered by ECC, we need this information. So, we hook the
 * ecc.read_oob and ecc.write_oob function pointers in the owning
 * struct mtd_info with our own functions. These hook functions set the
 * raw_oob_mode field so that, when control finally arrives here, we'll know
 * what to do.
 */
static int mxs_nand_ecc_read_oob(struct nand_chip *chip, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mxs_nand_info *nand_info = chip->priv;
	int column;

	/*
	 * First, fill in the OOB buffer. If we're doing a raw read, we need to
	 * get the bytes from the physical page. If we're not doing a raw read,
	 * we need to fill the buffer with set bits.
	 */
	if (nand_info->raw_oob_mode && nand_info->version > GPMI_VERSION_TYPE_MX23) {
		/*
		 * If control arrives here, we're doing a "raw" read. Send the
		 * command to read the conventional OOB and read it.
		 */
		chip->legacy.cmdfunc(chip, NAND_CMD_READ0, mtd->writesize, page);
		chip->legacy.read_buf(chip, chip->oob_poi, mtd->oobsize);
	} else {
		/*
		 * If control arrives here, we're not doing a "raw" read. Fill
		 * the OOB buffer with set bits and correct the block mark.
		 */
		memset(chip->oob_poi, 0xff, mtd->oobsize);

		column = nand_info->version == GPMI_VERSION_TYPE_MX23 ? 0 : mtd->writesize;
		chip->legacy.cmdfunc(chip, NAND_CMD_READ0, column, page);
		mxs_nand_read_buf(chip, chip->oob_poi, 1);
	}

	return 0;

}

/*
 * Write OOB data to NAND.
 */
static int mxs_nand_ecc_write_oob(struct nand_chip *chip, int page)
{
	/*
	 * There are fundamental incompatibilities between the i.MX GPMI NFC and
	 * the NAND Flash MTD model that make it essentially impossible to write
	 * the out-of-band bytes.
	 */

	printf("MXS NAND: Writing OOB isn't supported\n");
	return -EIO;
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
static int mxs_nand_block_bad(struct nand_chip *chip , loff_t ofs)
{
	return 0;
}

/*
 * Mark a block as bad in NAND.
 */
static int mxs_nand_block_markbad(struct nand_chip *chip , loff_t ofs)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mxs_nand_info *nand_info = chip->priv;
	int column, page, chipnr, status;
	uint8_t block_mark = 0;

	chipnr = (int)(ofs >> chip->chip_shift);
	nand_select_target(chip, chipnr);

	column = nand_info->version == GPMI_VERSION_TYPE_MX23 ? 0 : mtd->writesize;
	page = (int)(ofs >> chip->page_shift);
	/* Write the block mark. */
	chip->legacy.cmdfunc(chip, NAND_CMD_SEQIN, column, page);
	chip->legacy.write_buf(chip, &block_mark, 1);
	chip->legacy.cmdfunc(chip, NAND_CMD_PAGEPROG, -1, -1);

	/* Check if it worked. */
	status = chip->legacy.waitfunc(chip);

	nand_deselect_target(chip);

	if (status & NAND_STATUS_FAIL)
		return -EIO;

	return 0;
}

int mxs_nand_read_fcb_bch62(unsigned int block, void *buf, size_t size)
{
	struct nand_chip *chip;
	struct mxs_nand_info *nand_info;
	struct mtd_info *mtd = mxs_nand_mtd;
	int ret;
	int page;
	int flips = 0;
	uint8_t	*status;
	int i;

	if (!mtd)
		return -ENODEV;

	chip = mtd_to_nand(mtd);
	nand_info = chip->priv;

	nand_select_target(chip, 0);

	page = block * (mtd->erasesize / mtd->writesize);

	mxs_nand_mode_fcb_62bit(nand_info->bch_base);

	nand_read_page_op(chip, page, 0, NULL, 0);

	ret = mxs_nand_do_bch_read(chip, 0, BCH62_PAGESIZE, true, page);
	if (ret)
		goto out;

	/* Read DMA completed, now do the mark swapping. */
	mxs_nand_swap_block_mark(chip, nand_info->data_buf, nand_info->oob_buf);

	/* Loop over status bytes, accumulating ECC status. */
	status = nand_info->oob_buf + 32;

	for (i = 0; i < 8; i++) {
		switch (status[i]) {
		case 0x0:
			break;
		case 0xff:
			/*
			 * A status of 0xff means the chunk is erased, but due to
			 * the randomizer we see this as random data. Explicitly
			 * memset it.
			 */
			memset(nand_info->data_buf + 0x80 * i, 0xff, 0x80);
			break;
		case 0xfe:
			ret = -EBADMSG;
			goto out;
		default:
			flips += status[0];
			break;
		}
	}

	memcpy(buf, nand_info->data_buf, size);

out:
	mxs_nand_config_bch(chip, mtd->writesize + mtd->oobsize);
	nand_deselect_target(chip);

	return ret;
}

int mxs_nand_write_fcb_bch62(unsigned int block, void *buf, size_t size)
{
	struct nand_chip *chip;
	struct mtd_info *mtd = mxs_nand_mtd;
	struct mxs_nand_info *nand_info;
	struct mxs_dma_cmd *d;
	uint32_t channel;
	int ret = 0;
	int page;

	if (!mtd)
		return -ENODEV;

	if (size > BCH62_WRITESIZE)
		return -EINVAL;

	chip = mtd_to_nand(mtd);
	nand_info = chip->priv;
	channel = nand_info->dma_channel_base;

	mxs_nand_mode_fcb_62bit(nand_info->bch_base);

	nand_select_target(chip, 0);

	page = block * (mtd->erasesize / mtd->writesize);

	nand_prog_page_begin_op(chip, page, 0, NULL, 0);

	memset(nand_info->data_buf, 0x0, BCH62_WRITESIZE);
	memcpy(nand_info->data_buf, buf, size);

	/* Handle block mark swapping. */
	mxs_nand_swap_block_mark(chip, nand_info->data_buf, nand_info->oob_buf);

	/* Compile the DMA descriptor - write data. */
	d = mxs_nand_get_dma_desc(nand_info);
	d->data = MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_IRQ |
		      MXS_DMA_DESC_DEC_SEM | MXS_DMA_DESC_WAIT4END |
		      MXS_DMA_DESC_PIO_WORDS(6);

	d->address = 0;

	d->pio_words[0] = GPMI_CTRL0_COMMAND_MODE_WRITE |
			      GPMI_CTRL0_WORD_LENGTH |
			      GPMI_CTRL0_ADDRESS_NAND_DATA;
	d->pio_words[1] = 0;
	d->pio_words[2] = GPMI_ECCCTRL_ENABLE_ECC |
			      GPMI_ECCCTRL_ECC_CMD_ENCODE |
			      GPMI_ECCCTRL_BUFFER_MASK_BCH_PAGE;
	d->pio_words[3] = BCH62_PAGESIZE;
	d->pio_words[4] = (dma_addr_t)nand_info->data_buf;
	d->pio_words[5] = (dma_addr_t)nand_info->oob_buf;

	d->pio_words[2] |= GPMI_ECCCTRL_RANDOMIZER_ENABLE |
			       GPMI_ECCCTRL_RANDOMIZER_TYPE2;
	/*
	 * Write NAND page number needed to be randomized
	 * to GPMI_ECCCOUNT register.
	 *
	 * The value is between 0-255. For additional details
	 * check 9.6.6.4 of i.MX7D Applications Processor reference
	 */
	d->pio_words[3] |= (page % 256) << 16;

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel, nand_info->desc, nand_info->desc_index);
	if (ret) {
		dev_err(nand_info->dev, "MXS NAND: DMA write error: %d\n", ret);
		goto out;
	}

	ret = mxs_nand_wait_for_bch_complete(nand_info);
	if (ret) {
		dev_err(nand_info->dev, "MXS NAND: BCH write timeout\n");
		goto out;
	}

out:
	mxs_nand_return_dma_descs(nand_info);

	if (!ret)
		ret = nand_prog_page_end_op(chip);

	mxs_nand_config_bch(chip, mtd->writesize + mtd->oobsize);
	nand_deselect_target(chip);

	return ret;
}

/*
 * Nominally, the purpose of this function is to look for or create the bad
 * block table. In fact, since the we call this function at the very end of
 * the initialization process started by nand_scan(), and we doesn't have a
 * more formal mechanism, we "hook" this function to continue init process.
 *
 * At this point, the physical NAND Flash chips have been identified and
 * counted, so we know the physical geometry. This enables us to make some
 * important configuration decisions.
 *
 * The return value of this function propogates directly back to this driver's
 * call to nand_scan(). Anything other than zero will cause this driver to
 * tear everything down and declare failure.
 */
static int mxs_nand_scan_bbt(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct mxs_nand_info *nand_info = chip->priv;
	void __iomem *bch_regs = nand_info->bch_base;
	int ret;

	/* Reset BCH. Don't use SFTRST on MX23 due to Errata #2847 */
	ret = stmp_reset_block(bch_regs + BCH_CTRL,
				nand_info->version == GPMI_VERSION_TYPE_MX23);
	if (ret)
		return ret;

	mxs_nand_config_bch(chip, mtd->writesize + mtd->oobsize);

	/* Set *all* chip selects to use layout 0 */
	writel(0, bch_regs + BCH_LAYOUTSELECT);

	/* Enable BCH complete interrupt */
	writel(BCH_CTRL_COMPLETE_IRQ_EN, bch_regs + BCH_CTRL + STMP_OFFSET_REG_SET);

	/* Hook some operations at the MTD level. */
	if (mtd->_read_oob != mxs_nand_hook_read_oob) {
		nand_info->hooked_read_oob = mtd->_read_oob;
		mtd->_read_oob = mxs_nand_hook_read_oob;
	}

	if (mtd->_write_oob != mxs_nand_hook_write_oob) {
		nand_info->hooked_write_oob = mtd->_write_oob;
		mtd->_write_oob = mxs_nand_hook_write_oob;
	}

	/* We use the reference implementation for bad block management. */
	return nand_create_bbt(chip);
}

/*
 * Allocate DMA buffers
 */
static int mxs_nand_alloc_buffers(struct mxs_nand_info *nand_info)
{
	uint8_t *buf;
	const int size = NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE;

	/* DMA buffers */
	buf = dma_alloc_coherent(DMA_DEVICE_BROKEN, size, DMA_ADDRESS_BROKEN);
	if (!buf) {
		printf("MXS NAND: Error allocating DMA buffers\n");
		return -ENOMEM;
	}

	nand_info->data_buf = buf;
	nand_info->oob_buf = buf + NAND_MAX_PAGESIZE;

	/* Command buffers */
	nand_info->cmd_buf = dma_alloc_coherent(DMA_DEVICE_BROKEN,
						MXS_NAND_COMMAND_BUFFER_SIZE,
						DMA_ADDRESS_BROKEN);
	if (!nand_info->cmd_buf) {
		free(buf);
		printf("MXS NAND: Error allocating command buffers\n");
		return -ENOMEM;
	}
	nand_info->cmd_queue_len = 0;

	return 0;
}

/*
 * Initializes the NFC hardware.
 */
static int mxs_nand_hw_init(struct mxs_nand_info *info)
{
	void __iomem *gpmi_regs = info->io_base;
	void __iomem *bch_regs = info->bch_base;
	int ret;
	u32 val;

	info->desc = dma_alloc_coherent(DMA_DEVICE_BROKEN,
				   sizeof(struct mxs_dma_cmd) * MXS_NAND_DMA_DESCRIPTOR_COUNT,
				   DMA_ADDRESS_BROKEN);
	if (!info->desc)
		return -ENOMEM;

	/* Reset the GPMI block. */
	ret = stmp_reset_block(gpmi_regs + GPMI_CTRL0, 0);
	if (ret)
		return ret;

	val = readl(gpmi_regs + GPMI_VERSION);
	info->version = val >> GPMI_VERSION_MINOR_OFFSET;

	/* Reset BCH. Don't use SFTRST on MX23 due to Errata #2847 */
	ret = stmp_reset_block(bch_regs + BCH_CTRL,
				info->version == GPMI_VERSION_TYPE_MX23);
	if (ret)
		return ret;

	/*
	 * Choose NAND mode, set IRQ polarity, disable write protection and
	 * select BCH ECC.
	 */
	val = readl(gpmi_regs + GPMI_CTRL1);
	val &= ~GPMI_CTRL1_GPMI_MODE;
	val |= GPMI_CTRL1_ATA_IRQRDY_POLARITY | GPMI_CTRL1_DEV_RESET |
		GPMI_CTRL1_BCH_MODE;
	writel(val, gpmi_regs + GPMI_CTRL1);

	return 0;
}

static void mxs_nand_probe_dt(struct device *dev,
		              struct mxs_nand_info *nand_info)
{
	struct nand_chip *chip = &nand_info->nand_chip;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return;

	if (of_get_nand_on_flash_bbt(dev->of_node))
		chip->bbt_options |= NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB;
}

/**
 * struct gpmi_nfc_hardware_timing - GPMI hardware timing parameters.
 * @data_setup_in_cycles:      The data setup time, in cycles.
 * @data_hold_in_cycles:       The data hold time, in cycles.
 * @address_setup_in_cycles:   The address setup time, in cycles.
 * @device_busy_timeout:       The timeout waiting for NAND Ready/Busy,
 *                             this value is the number of cycles multiplied
 *                             by 4096.
 * @use_half_periods:          Indicates the clock is running slowly, so the
 *                             NFC DLL should use half-periods.
 * @sample_delay_factor:       The sample delay factor.
 * @wrn_dly_sel:               The delay on the GPMI write strobe.
 */
struct gpmi_nfc_hardware_timing {
	/* for GPMI_TIMING0 */
	uint8_t  data_setup_in_cycles;
	uint8_t  data_hold_in_cycles;
	uint8_t  address_setup_in_cycles;

	/* for GPMI_TIMING1 */
	uint16_t device_busy_timeout;

	/* for GPMI_CTRL1 */
	bool     use_half_periods;
	uint8_t  sample_delay_factor;
	uint8_t  wrn_dly_sel;
};

/**
 * struct timing_threshod - Timing threshold
 * @max_data_setup_cycles:       The maximum number of data setup cycles that
 *                               can be expressed in the hardware.
 * @internal_data_setup_in_ns:   The time, in ns, that the NFC hardware requires
 *                               for data read internal setup. In the Reference
 *                               Manual, see the chapter "High-Speed NAND
 *                               Timing" for more details.
 * @max_sample_delay_factor:     The maximum sample delay factor that can be
 *                               expressed in the hardware.
 * @max_dll_clock_period_in_ns:  The maximum period of the GPMI clock that the
 *                               sample delay DLL hardware can possibly work
 *                               with (the DLL is unusable with longer periods).
 *                               If the full-cycle period is greater than HALF
 *                               this value, the DLL must be configured to use
 *                               half-periods.
 * @max_dll_delay_in_ns:         The maximum amount of delay, in ns, that the
 *                               DLL can implement.
 */
struct timing_threshold {
	const unsigned int      max_data_setup_cycles;
	const unsigned int      internal_data_setup_in_ns;
	const unsigned int      max_sample_delay_factor;
	const unsigned int      max_dll_clock_period_in_ns;
	const unsigned int      max_dll_delay_in_ns;
};

static struct timing_threshold timing_default_threshold = {
	.max_data_setup_cycles       = 0xff,
	.internal_data_setup_in_ns   = 0,
	.max_sample_delay_factor     = 15,
	.max_dll_clock_period_in_ns  = 32,
	.max_dll_delay_in_ns         = 16,
};

/* Converts time in nanoseconds to cycles. */
static unsigned int ns_to_cycles(unsigned int time,
			unsigned int period, unsigned int min)
{
	unsigned int k;

	k = (time + period - 1) / period;
	return max(k, min);
}

/* Apply timing to current hardware conditions. */
static int mxs_nand_compute_hardware_timing(struct mxs_nand_info *info,
					struct gpmi_nfc_hardware_timing *hw)
{
	struct timing_threshold *nfc = &timing_default_threshold;
	struct nand_chip *chip = &info->nand_chip;
	struct nand_timing target = info->timing;
	unsigned long clock_frequency_in_hz;
	unsigned int clock_period_in_ns;
	bool dll_use_half_periods;
	unsigned int dll_delay_shift;
	unsigned int max_sample_delay_in_ns;
	unsigned int address_setup_in_cycles;
	unsigned int data_setup_in_ns;
	unsigned int data_setup_in_cycles;
	unsigned int data_hold_in_cycles;
	int ideal_sample_delay_in_ns;
	unsigned int sample_delay_factor;
	int tEYE;
	unsigned int min_prop_delay_in_ns = 5;
	unsigned int max_prop_delay_in_ns = 9;

	/*
	 * If there are multiple chips, we need to relax the timings to allow
	 * for signal distortion due to higher capacitance.
	 */
	if (nanddev_ntargets(&chip->base) > 2) {
		target.data_setup_in_ns    += 10;
		target.data_hold_in_ns     += 10;
		target.address_setup_in_ns += 10;
	} else if (nanddev_ntargets(&chip->base) > 1) {
		target.data_setup_in_ns    += 5;
		target.data_hold_in_ns     += 5;
		target.address_setup_in_ns += 5;
	}

	/* Inspect the clock. */
	clock_frequency_in_hz = clk_get_rate(info->clk);
	clock_period_in_ns    = NSEC_PER_SEC / clock_frequency_in_hz;

	/*
	 * The NFC quantizes setup and hold parameters in terms of clock cycles.
	 * Here, we quantize the setup and hold timing parameters to the
	 * next-highest clock period to make sure we apply at least the
	 * specified times.
	 *
	 * For data setup and data hold, the hardware interprets a value of zero
	 * as the largest possible delay. This is not what's intended by a zero
	 * in the input parameter, so we impose a minimum of one cycle.
	 */
	data_setup_in_cycles    = ns_to_cycles(target.data_setup_in_ns,
							clock_period_in_ns, 1);
	data_hold_in_cycles     = ns_to_cycles(target.data_hold_in_ns,
							clock_period_in_ns, 1);
	address_setup_in_cycles = ns_to_cycles(target.address_setup_in_ns,
							clock_period_in_ns, 0);

	/*
	 * The clock's period affects the sample delay in a number of ways:
	 *
	 * (1) The NFC HAL tells us the maximum clock period the sample delay
	 *     DLL can tolerate. If the clock period is greater than half that
	 *     maximum, we must configure the DLL to be driven by half periods.
	 *
	 * (2) We need to convert from an ideal sample delay, in ns, to a
	 *     "sample delay factor," which the NFC uses. This factor depends on
	 *     whether we're driving the DLL with full or half periods.
	 *     Paraphrasing the reference manual:
	 *
	 *         AD = SDF x 0.125 x RP
	 *
	 * where:
	 *
	 *     AD   is the applied delay, in ns.
	 *     SDF  is the sample delay factor, which is dimensionless.
	 *     RP   is the reference period, in ns, which is a full clock period
	 *          if the DLL is being driven by full periods, or half that if
	 *          the DLL is being driven by half periods.
	 *
	 * Let's re-arrange this in a way that's more useful to us:
	 *
	 *                        8
	 *         SDF  =  AD x ----
	 *                       RP
	 *
	 * The reference period is either the clock period or half that, so this
	 * is:
	 *
	 *                        8       AD x DDF
	 *         SDF  =  AD x -----  =  --------
	 *                      f x P        P
	 *
	 * where:
	 *
	 *       f  is 1 or 1/2, depending on how we're driving the DLL.
	 *       P  is the clock period.
	 *     DDF  is the DLL Delay Factor, a dimensionless value that
	 *          incorporates all the constants in the conversion.
	 *
	 * DDF will be either 8 or 16, both of which are powers of two. We can
	 * reduce the cost of this conversion by using bit shifts instead of
	 * multiplication or division. Thus:
	 *
	 *                 AD << DDS
	 *         SDF  =  ---------
	 *                     P
	 *
	 *     or
	 *
	 *         AD  =  (SDF >> DDS) x P
	 *
	 * where:
	 *
	 *     DDS  is the DLL Delay Shift, the logarithm to base 2 of the DDF.
	 */
	if (clock_period_in_ns > (nfc->max_dll_clock_period_in_ns >> 1)) {
		dll_use_half_periods = true;
		dll_delay_shift      = 3 + 1;
	} else {
		dll_use_half_periods = false;
		dll_delay_shift      = 3;
	}

	/*
	 * Compute the maximum sample delay the NFC allows, under current
	 * conditions. If the clock is running too slowly, no sample delay is
	 * possible.
	 */
	if (clock_period_in_ns > nfc->max_dll_clock_period_in_ns) {
		max_sample_delay_in_ns = 0;
	} else {
		/*
		 * Compute the delay implied by the largest sample delay factor
		 * the NFC allows.
		 */
		max_sample_delay_in_ns =
			(nfc->max_sample_delay_factor * clock_period_in_ns) >>
								dll_delay_shift;

		/*
		 * Check if the implied sample delay larger than the NFC
		 * actually allows.
		 */
		if (max_sample_delay_in_ns > nfc->max_dll_delay_in_ns)
			max_sample_delay_in_ns = nfc->max_dll_delay_in_ns;
	}

	/*
	 * Fold the read setup time required by the NFC into the ideal
	 * sample delay.
	 */
	ideal_sample_delay_in_ns = target.gpmi_sample_delay_in_ns +
					nfc->internal_data_setup_in_ns;

	/*
	 * The ideal sample delay may be greater than the maximum
	 * allowed by the NFC. If so, we can trade off sample delay time
	 * for more data setup time.
	 *
	 * In each iteration of the following loop, we add a cycle to
	 * the data setup time and subtract a corresponding amount from
	 * the sample delay until we've satisified the constraints or
	 * can't do any better.
	 */
	while ((ideal_sample_delay_in_ns > max_sample_delay_in_ns) &&
		(data_setup_in_cycles < nfc->max_data_setup_cycles)) {

		data_setup_in_cycles++;
		ideal_sample_delay_in_ns -= clock_period_in_ns;

		if (ideal_sample_delay_in_ns < 0)
			ideal_sample_delay_in_ns = 0;
	}

	/*
	 * Compute the sample delay factor that corresponds most closely
	 * to the ideal sample delay. If the result is too large for the
	 * NFC, use the maximum value.
	 *
	 * Notice that we use the ns_to_cycles function to compute the
	 * sample delay factor. We do this because the form of the
	 * computation is the same as that for calculating cycles.
	 */
	sample_delay_factor =
		ns_to_cycles(
			ideal_sample_delay_in_ns << dll_delay_shift,
						clock_period_in_ns, 0);

	if (sample_delay_factor > nfc->max_sample_delay_factor)
		sample_delay_factor = nfc->max_sample_delay_factor;

	/* Skip to the part where we return our results. */
	goto return_results;

	/*
	 * If control arrives here, we have more detailed timing information,
	 * so we can use a better algorithm.
	 */

	/*
	 * Fold the read setup time required by the NFC into the maximum
	 * propagation delay.
	 */
	max_prop_delay_in_ns += nfc->internal_data_setup_in_ns;

	/*
	 * Earlier, we computed the number of clock cycles required to satisfy
	 * the data setup time. Now, we need to know the actual nanoseconds.
	 */
	data_setup_in_ns = clock_period_in_ns * data_setup_in_cycles;

	/*
	 * Compute tEYE, the width of the data eye when reading from the NAND
	 * Flash. The eye width is fundamentally determined by the data setup
	 * time, perturbed by propagation delays and some characteristics of the
	 * NAND Flash device.
	 *
	 * start of the eye = max_prop_delay + tREA
	 * end of the eye   = min_prop_delay + tRHOH + data_setup
	 */
	tEYE = (int)min_prop_delay_in_ns + (int)target.tRHOH_in_ns +
							(int)data_setup_in_ns;

	tEYE -= (int)max_prop_delay_in_ns + (int)target.tREA_in_ns;

	/*
	 * The eye must be open. If it's not, we can try to open it by
	 * increasing its main forcer, the data setup time.
	 *
	 * In each iteration of the following loop, we increase the data setup
	 * time by a single clock cycle. We do this until either the eye is
	 * open or we run into NFC limits.
	 */
	while ((tEYE <= 0) &&
			(data_setup_in_cycles < nfc->max_data_setup_cycles)) {
		/* Give a cycle to data setup. */
		data_setup_in_cycles++;
		/* Synchronize the data setup time with the cycles. */
		data_setup_in_ns += clock_period_in_ns;
		/* Adjust tEYE accordingly. */
		tEYE += clock_period_in_ns;
	}

	/*
	 * When control arrives here, the eye is open. The ideal time to sample
	 * the data is in the center of the eye:
	 *
	 *     end of the eye + start of the eye
	 *     ---------------------------------  -  data_setup
	 *                    2
	 *
	 * After some algebra, this simplifies to the code immediately below.
	 */
	ideal_sample_delay_in_ns =
		((int)max_prop_delay_in_ns +
			(int)target.tREA_in_ns +
				(int)min_prop_delay_in_ns +
					(int)target.tRHOH_in_ns -
						(int)data_setup_in_ns) >> 1;

	/*
	 * The following figure illustrates some aspects of a NAND Flash read:
	 *
	 *
	 *           __                   _____________________________________
	 * RDN         \_________________/
	 *
	 *                                         <---- tEYE ----->
	 *                                        /-----------------\
	 * Read Data ----------------------------<                   >---------
	 *                                        \-----------------/
	 *             ^                 ^                 ^              ^
	 *             |                 |                 |              |
	 *             |<--Data Setup -->|<--Delay Time -->|              |
	 *             |                 |                 |              |
	 *             |                 |                                |
	 *             |                 |<--   Quantized Delay Time   -->|
	 *             |                 |                                |
	 *
	 *
	 * We have some issues we must now address:
	 *
	 * (1) The *ideal* sample delay time must not be negative. If it is, we
	 *     jam it to zero.
	 *
	 * (2) The *ideal* sample delay time must not be greater than that
	 *     allowed by the NFC. If it is, we can increase the data setup
	 *     time, which will reduce the delay between the end of the data
	 *     setup and the center of the eye. It will also make the eye
	 *     larger, which might help with the next issue...
	 *
	 * (3) The *quantized* sample delay time must not fall either before the
	 *     eye opens or after it closes (the latter is the problem
	 *     illustrated in the above figure).
	 */

	/* Jam a negative ideal sample delay to zero. */
	if (ideal_sample_delay_in_ns < 0)
		ideal_sample_delay_in_ns = 0;

	/*
	 * Extend the data setup as needed to reduce the ideal sample delay
	 * below the maximum permitted by the NFC.
	 */
	while ((ideal_sample_delay_in_ns > max_sample_delay_in_ns) &&
			(data_setup_in_cycles < nfc->max_data_setup_cycles)) {

		/* Give a cycle to data setup. */
		data_setup_in_cycles++;
		/* Synchronize the data setup time with the cycles. */
		data_setup_in_ns += clock_period_in_ns;
		/* Adjust tEYE accordingly. */
		tEYE += clock_period_in_ns;

		/*
		 * Decrease the ideal sample delay by one half cycle, to keep it
		 * in the middle of the eye.
		 */
		ideal_sample_delay_in_ns -= (clock_period_in_ns >> 1);

		/* Jam a negative ideal sample delay to zero. */
		if (ideal_sample_delay_in_ns < 0)
			ideal_sample_delay_in_ns = 0;
	}

	/*
	 * Compute the sample delay factor that corresponds to the ideal sample
	 * delay. If the result is too large, then use the maximum allowed
	 * value.
	 *
	 * Notice that we use the ns_to_cycles function to compute the sample
	 * delay factor. We do this because the form of the computation is the
	 * same as that for calculating cycles.
	 */
	sample_delay_factor =
		ns_to_cycles(ideal_sample_delay_in_ns << dll_delay_shift,
							clock_period_in_ns, 0);

	if (sample_delay_factor > nfc->max_sample_delay_factor)
		sample_delay_factor = nfc->max_sample_delay_factor;

	/*
	 * These macros conveniently encapsulate a computation we'll use to
	 * continuously evaluate whether or not the data sample delay is inside
	 * the eye.
	 */
	#define IDEAL_DELAY  ((int) ideal_sample_delay_in_ns)

	#define QUANTIZED_DELAY  \
		((int) ((sample_delay_factor * clock_period_in_ns) >> \
							dll_delay_shift))

	#define DELAY_ERROR  (abs(QUANTIZED_DELAY - IDEAL_DELAY))

	#define SAMPLE_IS_NOT_WITHIN_THE_EYE  (DELAY_ERROR > (tEYE >> 1))

	/*
	 * While the quantized sample time falls outside the eye, reduce the
	 * sample delay or extend the data setup to move the sampling point back
	 * toward the eye. Do not allow the number of data setup cycles to
	 * exceed the maximum allowed by the NFC.
	 */
	while (SAMPLE_IS_NOT_WITHIN_THE_EYE &&
			(data_setup_in_cycles < nfc->max_data_setup_cycles)) {
		/*
		 * If control arrives here, the quantized sample delay falls
		 * outside the eye. Check if it's before the eye opens, or after
		 * the eye closes.
		 */
		if (QUANTIZED_DELAY > IDEAL_DELAY) {
			/*
			 * If control arrives here, the quantized sample delay
			 * falls after the eye closes. Decrease the quantized
			 * delay time and then go back to re-evaluate.
			 */
			if (sample_delay_factor != 0)
				sample_delay_factor--;
			continue;
		}

		/*
		 * If control arrives here, the quantized sample delay falls
		 * before the eye opens. Shift the sample point by increasing
		 * data setup time. This will also make the eye larger.
		 */

		/* Give a cycle to data setup. */
		data_setup_in_cycles++;
		/* Synchronize the data setup time with the cycles. */
		data_setup_in_ns += clock_period_in_ns;
		/* Adjust tEYE accordingly. */
		tEYE += clock_period_in_ns;

		/*
		 * Decrease the ideal sample delay by one half cycle, to keep it
		 * in the middle of the eye.
		 */
		ideal_sample_delay_in_ns -= (clock_period_in_ns >> 1);

		/* ...and one less period for the delay time. */
		ideal_sample_delay_in_ns -= clock_period_in_ns;

		/* Jam a negative ideal sample delay to zero. */
		if (ideal_sample_delay_in_ns < 0)
			ideal_sample_delay_in_ns = 0;

		/*
		 * We have a new ideal sample delay, so re-compute the quantized
		 * delay.
		 */
		sample_delay_factor =
			ns_to_cycles(
				ideal_sample_delay_in_ns << dll_delay_shift,
							clock_period_in_ns, 0);

		if (sample_delay_factor > nfc->max_sample_delay_factor)
			sample_delay_factor = nfc->max_sample_delay_factor;
	}

	/* Control arrives here when we're ready to return our results. */
return_results:
	hw->data_setup_in_cycles    = data_setup_in_cycles;
	hw->data_hold_in_cycles     = data_hold_in_cycles;
	hw->address_setup_in_cycles = address_setup_in_cycles;
	hw->use_half_periods        = dll_use_half_periods;
	hw->sample_delay_factor     = sample_delay_factor;
	hw->device_busy_timeout     = 0x500;
	hw->wrn_dly_sel             = BV_GPMI_CTRL1_WRN_DLY_SEL_4_TO_8NS;

	/* Return success. */
	return 0;
}

/*
 * <1> Firstly, we should know what's the GPMI-clock means.
 *     The GPMI-clock is the internal clock in the gpmi nand controller.
 *     If you set 100MHz to gpmi nand controller, the GPMI-clock's period
 *     is 10ns. Mark the GPMI-clock's period as GPMI-clock-period.
 *
 * <2> Secondly, we should know what's the frequency on the nand chip pins.
 *     The frequency on the nand chip pins is derived from the GPMI-clock.
 *     We can get it from the following equation:
 *
 *         F = G / (DS + DH)
 *
 *         F  : the frequency on the nand chip pins.
 *         G  : the GPMI clock, such as 100MHz.
 *         DS : GPMI_TIMING0:DATA_SETUP
 *         DH : GPMI_TIMING0:DATA_HOLD
 *
 * <3> Thirdly, when the frequency on the nand chip pins is above 33MHz,
 *     the nand EDO(extended Data Out) timing could be applied.
 *     The GPMI implements a feedback read strobe to sample the read data.
 *     The feedback read strobe can be delayed to support the nand EDO timing
 *     where the read strobe may deasserts before the read data is valid, and
 *     read data is valid for some time after read strobe.
 *
 *     The following figure illustrates some aspects of a NAND Flash read:
 *
 *                   |<---tREA---->|
 *                   |             |
 *                   |         |   |
 *                   |<--tRP-->|   |
 *                   |         |   |
 *                  __          ___|__________________________________
 *     RDN            \________/   |
 *                                 |
 *                                 /---------\
 *     Read Data    --------------<           >---------
 *                                 \---------/
 *                                |     |
 *                                |<-D->|
 *     FeedbackRDN  ________             ____________
 *                          \___________/
 *
 *          D stands for delay, set in the GPMI_CTRL1:RDN_DELAY.
 *
 *
 * <4> Now, we begin to describe how to compute the right RDN_DELAY.
 *
 *  4.1) From the aspect of the nand chip pins:
 *        Delay = (tREA + C - tRP)               {1}
 *
 *        tREA : the maximum read access time. From the ONFI nand standards,
 *               we know that tREA is 16ns in mode 5, tREA is 20ns is mode 4.
 *               Please check it in : www.onfi.org
 *        C    : a constant for adjust the delay. default is 4.
 *        tRP  : the read pulse width.
 *               Specified by the GPMI_TIMING0:DATA_SETUP:
 *                    tRP = (GPMI-clock-period) * DATA_SETUP
 *
 *  4.2) From the aspect of the GPMI nand controller:
 *         Delay = RDN_DELAY * 0.125 * RP        {2}
 *
 *         RP   : the DLL reference period.
 *            if (GPMI-clock-period > DLL_THRETHOLD)
 *                   RP = GPMI-clock-period / 2;
 *            else
 *                   RP = GPMI-clock-period;
 *
 *            Set the GPMI_CTRL1:HALF_PERIOD if GPMI-clock-period
 *            is greater DLL_THRETHOLD. In other SOCs, the DLL_THRETHOLD
 *            is 16ns, but in mx6q, we use 12ns.
 *
 *  4.3) since {1} equals {2}, we get:
 *
 *                    (tREA + 4 - tRP) * 8
 *         RDN_DELAY = ---------------------     {3}
 *                           RP
 *
 *  4.4) We only support the fastest asynchronous mode of ONFI nand.
 *       For some ONFI nand, the mode 4 is the fastest mode;
 *       while for some ONFI nand, the mode 5 is the fastest mode.
 *       So we only support the mode 4 and mode 5. It is no need to
 *       support other modes.
 */
static void mxs_nand_compute_edo_timing(struct mxs_nand_info *info,
			struct gpmi_nfc_hardware_timing *hw, int mode)
{
	unsigned long rate = clk_get_rate(info->clk);
	int dll_threshold = 12;
	unsigned long delay;
	unsigned long clk_period;
	int t_rea;
	int c = 4;
	int t_rp;
	int rp;

	/*
	 * [1] for GPMI_TIMING0:
	 *     The async mode requires 40MHz for mode 4, 50MHz for mode 5.
	 *     The GPMI can support 100MHz at most. So if we want to
	 *     get the 40MHz or 50MHz, we have to set DS=1, DH=1.
	 *     Set the ADDRESS_SETUP to 0 in mode 4.
	 */
	hw->data_setup_in_cycles = 1;
	hw->data_hold_in_cycles = 1;
	hw->address_setup_in_cycles = ((mode == 5) ? 1 : 0);

	/* [2] for GPMI_TIMING1 */
	hw->device_busy_timeout = 0x9000;

	/* [3] for GPMI_CTRL1 */
	hw->wrn_dly_sel = BV_GPMI_CTRL1_WRN_DLY_SEL_NO_DELAY;

	/*
	 * Enlarge 10 times for the numerator and denominator in {3}.
	 * This make us to get more accurate result.
	 */
	clk_period = NSEC_PER_SEC / (rate / 10);
	dll_threshold *= 10;
	t_rea = ((mode == 5) ? 16 : 20) * 10;
	c *= 10;

	t_rp = clk_period * 1; /* DATA_SETUP is 1 */

	if (clk_period > dll_threshold) {
		hw->use_half_periods = 1;
		rp = clk_period / 2;
	} else {
		hw->use_half_periods = 0;
		rp = clk_period;
	}

	/*
	 * Multiply the numerator with 10, we could do a round off:
	 *      7.8 round up to 8; 7.4 round down to 7.
	 */
	delay  = (((t_rea + c - t_rp) * 8) * 10) / rp;
	delay = (delay + 5) / 10;

	hw->sample_delay_factor = delay;
}

static int mxs_nand_enable_edo_mode(struct mxs_nand_info *info)
{
	struct nand_chip *chip = &info->nand_chip;
	uint8_t feature[ONFI_SUBFEATURE_PARAM_LEN] = {};
	int ret, mode;

	if (!chip->parameters.onfi)
		return -ENOENT;

	mode = onfi_get_async_timing_mode(chip);

	/* We only support the timing mode 4 and mode 5. */
	if (mode & ONFI_TIMING_MODE_5)
		mode = 5;
	else if (mode & ONFI_TIMING_MODE_4)
		mode = 4;
	else
		return -EINVAL;

	chip->legacy.select_chip(chip, 0);

	if (nand_supports_set_features(chip, ONFI_FEATURE_ADDR_TIMING_MODE)) {

		/* [1] send SET FEATURE commond to NAND */
		feature[0] = mode;

		ret = nand_set_features(chip, ONFI_FEATURE_ADDR_TIMING_MODE, feature);
		if (ret)
			goto err_out;

		/* [2] send GET FEATURE command to double-check the timing mode */
		ret = nand_get_features(chip, ONFI_FEATURE_ADDR_TIMING_MODE, feature);
		if (ret || feature[0] != mode)
			goto err_out;
	}

	chip->legacy.select_chip(chip, -1);

	/* [3] set the main IO clock, 100MHz for mode 5, 80MHz for mode 4. */
	clk_disable(info->clk);
	clk_set_rate(info->clk, (mode == 5) ? 100000000 : 80000000);
	clk_enable(info->clk);

	dev_dbg(info->dev, "using asynchronous EDO mode %d\n", mode);

	return mode;

err_out:
	chip->legacy.select_chip(chip, -1);

	return -EINVAL;
}

static void mxs_nand_setup_timing(struct mxs_nand_info *info)
{
	void __iomem *gpmi_regs = info->io_base;
	uint32_t reg;
	struct gpmi_nfc_hardware_timing hw;
	int mode;

	mode = mxs_nand_enable_edo_mode(info);
	if (mode >= 0)
		mxs_nand_compute_edo_timing(info, &hw, mode);
	else
		mxs_nand_compute_hardware_timing(info, &hw);

	writel(GPMI_TIMING0_ADDRESS_SETUP(hw.address_setup_in_cycles) |
		GPMI_TIMING0_DATA_HOLD(hw.data_hold_in_cycles) |
		GPMI_TIMING0_DATA_SETUP(hw.data_setup_in_cycles),
		gpmi_regs + GPMI_TIMING0);

	writel(GPMI_TIMING1_BUSY_TIMEOUT(hw.device_busy_timeout),
		gpmi_regs + GPMI_TIMING1);

	reg = readl(gpmi_regs + GPMI_CTRL1);

	reg &= ~GPMI_CTRL1_WRN_DLY(3);
	reg |= GPMI_CTRL1_WRN_DLY(hw.wrn_dly_sel);
	/* DLL_ENABLE must be set to 0 when setting RDN_DELAY or HALF_PERIOD. */
	reg &= ~GPMI_CTRL1_DLL_ENABLE;
	reg &= ~GPMI_CTRL1_RDN_DELAY(0xf);
	reg |= GPMI_CTRL1_RDN_DELAY(hw.sample_delay_factor);
	reg &= ~GPMI_CTRL1_HALF_PERIOD;
	if (hw.use_half_periods)
		reg |= GPMI_CTRL1_HALF_PERIOD;

	writel(reg, gpmi_regs + GPMI_CTRL1);

	if (hw.sample_delay_factor) {
		writel(GPMI_CTRL1_DLL_ENABLE, gpmi_regs + GPMI_CTRL1_SET);
		/*
		 * After we enable the GPMI DLL, we have to wait 64 clock
		 * cycles before we can use the GPMI. Assume 1MHz as lowest
		 * bus clock.
		 */
		udelay(64);
	}
}

static int mxs_nand_probe(struct device *dev)
{
	struct resource *iores;
	struct mxs_nand_info *nand_info;
	struct nand_chip *chip;
	struct mtd_info *mtd;
	enum gpmi_type type;
	int err;

	/* We expect only one */
	if (mxs_nand_mtd)
		return -EBUSY;

	type = (enum gpmi_type)device_get_match_data(dev);

	nand_info = kzalloc(sizeof(struct mxs_nand_info), GFP_KERNEL);
	if (!nand_info) {
		printf("MXS NAND: Failed to allocate private data\n");
		return -ENOMEM;
	}

	nand_info->dev = dev;

	mxs_nand_probe_dt(dev, nand_info);

	nand_info->type = type;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	nand_info->io_base = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	nand_info->bch_base = IOMEM(iores->start);

	nand_info->clk = clk_get(dev, NULL);
	if (IS_ERR(nand_info->clk))
		return PTR_ERR(nand_info->clk);

	clk_enable(nand_info->clk);

	if (mxs_nand_is_imx6(nand_info)) {
		clk_disable(nand_info->clk);
		clk_set_rate(nand_info->clk, 22000000);
		clk_enable(nand_info->clk);
		nand_info->dma_channel_base = 0;
	} else {
		nand_info->dma_channel_base = MXS_DMA_CHANNEL_AHB_APBH_GPMI0;
		clk_set_rate(nand_info->clk, 22000000);
	}

	err = mxs_nand_alloc_buffers(nand_info);
	if (err)
		goto err1;

	err = mxs_nand_hw_init(nand_info);
	if (err)
		goto err2;

	/* structures must be linked */
	chip = &nand_info->nand_chip;
	mtd = nand_to_mtd(chip);
	mtd->dev.parent = dev;

	chip->priv = nand_info;

	chip->legacy.cmd_ctrl		= mxs_nand_cmd_ctrl;

	chip->legacy.dev_ready		= mxs_nand_device_ready;
	chip->legacy.select_chip	= mxs_nand_select_chip;
	chip->legacy.block_bad		= mxs_nand_block_bad;
	chip->legacy.block_markbad	= mxs_nand_block_markbad;

	chip->legacy.read_byte		= mxs_nand_read_byte;

	chip->legacy.read_buf		= mxs_nand_read_buf;
	chip->legacy.write_buf		= mxs_nand_write_buf;

	chip->ecc.read_page	= mxs_nand_ecc_read_page;
	chip->ecc.write_page	= mxs_nand_ecc_write_page;
	chip->ecc.read_oob	= mxs_nand_ecc_read_oob;
	chip->ecc.write_oob	= mxs_nand_ecc_write_oob;

	chip->ecc.engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;

	/* first scan to find the device and get the page size */
	err = nand_scan_ident(chip, 4, NULL);
	if (err)
		goto err2;

	err = mxs_nand_calc_geo(chip);
	if (err)
		goto err2;

	if ((13 * chip->ecc.strength % 8) == 0) {
		chip->ecc.read_subpage = gpmi_ecc_read_subpage;
		chip->options |= NAND_SUBPAGE_READ;
	}

	chip->options |= NAND_NO_SUBPAGE_WRITE | NAND_SKIP_BBTSCAN;

	mxs_nand_setup_timing(nand_info);

	mtd_set_ooblayout(mtd, &mxs_nand_ooblayout_ops);

	/* second phase scan */
	err = nand_scan_tail(chip);
	if (err)
		goto err2;

	mxs_nand_scan_bbt(chip);

	err = add_mtd_nand_device(mtd, "nand");
	if (err)
		goto err2;

	mxs_nand_mtd = mtd;

	return 0;
err2:
	free(nand_info->data_buf);
	free(nand_info->cmd_buf);
err1:
	free(nand_info);

	mxs_nand_mtd = NULL;

	return err;
}

static __maybe_unused struct of_device_id gpmi_dt_ids[] = {
	{
		.compatible = "fsl,imx23-gpmi-nand",
		.data = (void *)GPMI_MXS,
	}, {
		.compatible = "fsl,imx28-gpmi-nand",
		.data = (void *)GPMI_MXS,
	}, {
		.compatible = "fsl,imx6q-gpmi-nand",
		.data = (void *)GPMI_IMX6,
	}, {
		.compatible = "fsl,imx7d-gpmi-nand",
		.data = (void *)GPMI_IMX6,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, gpmi_dt_ids);

static struct driver mxs_nand_driver = {
	.name  = "mxs_nand",
	.probe = mxs_nand_probe,
	.of_compatible = DRV_OF_COMPAT(gpmi_dt_ids),
};
device_platform_driver(mxs_nand_driver);

MODULE_AUTHOR("Denx Software Engeneering and Wolfram Sang");
MODULE_DESCRIPTION("MXS NAND MTD driver");
MODULE_LICENSE("GPL");
