// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2016 Pengutronix, Steffen Trumtrar <kernel@pengutronix.de>
 *
 * The driver is based on information gathered from
 * drivers/mxc/security/imx_scc.c which can be found in
 * the Freescale linux-2.6-imx.git in the imx_2.6.35_maintain branch.
 */
#include <common.h>
#include <clock.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <crypto.h>
#include <linux/barebox-wrapper.h>
#include <linux/clk.h>
#include <crypto/des.h>

#include "scc.h"

/* Secure Memory (SCM) registers */
#define SCC_SCM_RED_START		0x0000
#define SCC_SCM_BLACK_START		0x0004
#define SCC_SCM_LENGTH			0x0008
#define SCC_SCM_CTRL			0x000C
#define SCC_SCM_STATUS			0x0010
#define SCC_SCM_ERROR_STATUS		0x0014
#define SCC_SCM_INTR_CTRL		0x0018
#define SCC_SCM_CFG			0x001C
#define SCC_SCM_INIT_VECTOR_0		0x0020
#define SCC_SCM_INIT_VECTOR_1		0x0024
#define SCC_SCM_RED_MEMORY		0x0400
#define SCC_SCM_BLACK_MEMORY		0x0800

/* Security Monitor (SMN) Registers */
#define SCC_SMN_STATUS			0x1000
#define SCC_SMN_COMMAND			0x1004
#define SCC_SMN_SEQ_START		0x1008
#define SCC_SMN_SEQ_END			0x100C
#define SCC_SMN_SEQ_CHECK		0x1010
#define SCC_SMN_BIT_COUNT		0x1014
#define SCC_SMN_BITBANK_INC_SIZE	0x1018
#define SCC_SMN_BITBANK_DECREMENT	0x101C
#define SCC_SMN_COMPARE_SIZE		0x1020
#define SCC_SMN_PLAINTEXT_CHECK		0x1024
#define SCC_SMN_CIPHERTEXT_CHECK	0x1028
#define SCC_SMN_TIMER_IV		0x102C
#define SCC_SMN_TIMER_CONTROL		0x1030
#define SCC_SMN_DEBUG_DETECT_STAT	0x1034
#define SCC_SMN_TIMER			0x1038

#define SCC_SCM_CTRL_START_CIPHER	BIT(2)
#define SCC_SCM_CTRL_CBC_MODE		BIT(1)
#define SCC_SCM_CTRL_DECRYPT_MODE	BIT(0)

#define SCC_SCM_STATUS_LEN_ERR		BIT(12)
#define SCC_SCM_STATUS_SMN_UNBLOCKED	BIT(11)
#define SCC_SCM_STATUS_CIPHERING_DONE	BIT(10)
#define SCC_SCM_STATUS_ZEROIZING_DONE	BIT(9)
#define SCC_SCM_STATUS_INTR_STATUS	BIT(8)
#define SCC_SCM_STATUS_SEC_KEY		BIT(7)
#define SCC_SCM_STATUS_INTERNAL_ERR	BIT(6)
#define SCC_SCM_STATUS_BAD_SEC_KEY	BIT(5)
#define SCC_SCM_STATUS_ZEROIZE_FAIL	BIT(4)
#define SCC_SCM_STATUS_SMN_BLOCKED	BIT(3)
#define SCC_SCM_STATUS_CIPHERING	BIT(2)
#define SCC_SCM_STATUS_ZEROIZING	BIT(1)
#define SCC_SCM_STATUS_BUSY		BIT(0)

#define SCC_SMN_STATUS_STATE_MASK	0x0000001F
#define SCC_SMN_STATE_START		0x0
/* The SMN is zeroizing its RAM during reset */
#define SCC_SMN_STATE_ZEROIZE_RAM	0x5
/* SMN has passed internal checks */
#define SCC_SMN_STATE_HEALTH_CHECK	0x6
/* Fatal Security Violation. SMN is locked, SCM is inoperative. */
#define SCC_SMN_STATE_FAIL		0x9
/* SCC is in secure state. SCM is using secret key. */
#define SCC_SMN_STATE_SECURE		0xA
/* SCC is not secure. SCM is using default key. */
#define SCC_SMN_STATE_NON_SECURE	0xC

#define SCC_SCM_INTR_CTRL_ZEROIZE_MEM	BIT(2)
#define SCC_SCM_INTR_CTRL_CLR_INTR	BIT(1)
#define SCC_SCM_INTR_CTRL_MASK_INTR	BIT(0)

/* Size, in blocks, of Red memory. */
#define SCC_SCM_CFG_BLACK_SIZE_MASK	0x07fe0000
#define SCC_SCM_CFG_BLACK_SIZE_SHIFT	17
/* Size, in blocks, of Black memory. */
#define SCC_SCM_CFG_RED_SIZE_MASK	0x0001ff80
#define SCC_SCM_CFG_RED_SIZE_SHIFT	7
/* Number of bytes per block. */
#define SCC_SCM_CFG_BLOCK_SIZE_MASK	0x0000007f

#define SCC_SMN_COMMAND_TAMPER_LOCK	BIT(4)
#define SCC_SMN_COMMAND_CLR_INTR	BIT(3)
#define SCC_SMN_COMMAND_CLR_BIT_BANK	BIT(2)
#define SCC_SMN_COMMAND_EN_INTR		BIT(1)
#define SCC_SMN_COMMAND_SET_SOFTWARE_ALARM  BIT(0)

#define SCC_KEY_SLOTS			20
#define SCC_MAX_KEY_SIZE		32
#define SCC_KEY_SLOT_SIZE		32

#define SCC_CRC_CCITT_START		0xFFFF

/*
 * Offset into each RAM of the base of the area which is not
 * used for Stored Keys.
 */
#define SCC_NON_RESERVED_OFFSET	(SCC_KEY_SLOTS * SCC_KEY_SLOT_SIZE)

/* Fixed padding for appending to plaintext to fill out a block */
static char scc_block_padding[8] = { 0x80, 0, 0, 0, 0, 0, 0, 0 };

struct imx_scc {
	struct device	*dev;
	void __iomem		*base;
	struct clk		*clk;
	struct ablkcipher_request *req;
	unsigned int		block_size_bytes;
	unsigned int		black_ram_size_blocks;
	unsigned int		memory_size_bytes;
	unsigned int		bytes_remaining;

	void __iomem		*red_memory;
	void __iomem		*black_memory;
};

struct imx_scc_ctx {
	struct imx_scc		*scc;
	unsigned int		offset;
	unsigned int		size;
	unsigned int		ctrl;
};

static struct imx_scc *scc_dev;

static int imx_scc_get_data(struct imx_scc_ctx *ctx,
			    struct ablkcipher_request *ablkreq)
{
	struct imx_scc *scc = ctx->scc;
	void __iomem *from;

	if (ctx->ctrl & SCC_SCM_CTRL_DECRYPT_MODE)
		from = scc->red_memory;
	else
		from = scc->black_memory;

	memcpy(ablkreq->dst, from + ctx->offset, ctx->size);

	pr_debug("GET_DATA:\n");
	pr_memory_display(MSG_DEBUG, from, 0, ctx->size, 0x40 >> 3, 0);

	ctx->offset += ctx->size;

	if (ctx->offset < ablkreq->nbytes)
		return -EINPROGRESS;

	return 0;
}

static int imx_scc_ablkcipher_req_init(struct ablkcipher_request *req,
				       struct imx_scc_ctx *ctx)
{
	ctx->size = 0;
	ctx->offset = 0;

	return 0;
}

static int imx_scc_put_data(struct imx_scc_ctx *ctx,
			    struct ablkcipher_request *req)
{
	u8 padding_buffer[sizeof(u16) + sizeof(scc_block_padding)];
	size_t len = min(req->nbytes - ctx->offset, ctx->scc->bytes_remaining);
	unsigned int padding_byte_count = 0;
	struct imx_scc *scc = ctx->scc;
	void __iomem *to;

	if (ctx->ctrl & SCC_SCM_CTRL_DECRYPT_MODE)
		to = scc->black_memory;
	else
		to = scc->red_memory;

	if (ctx->ctrl & SCC_SCM_CTRL_CBC_MODE) {
		dev_dbg(scc->dev, "set IV@0x%p\n", scc->base + SCC_SCM_INIT_VECTOR_0);
		memcpy(scc->base + SCC_SCM_INIT_VECTOR_0, req->info,
		       scc->block_size_bytes);
	}

	memcpy(to, req->src + ctx->offset, len);

	ctx->size = len;

	scc->bytes_remaining -= len;

	padding_byte_count = ((len + scc->block_size_bytes - 1) &
			      ~(scc->block_size_bytes-1)) - len;

	if (padding_byte_count) {
		memcpy(padding_buffer, scc_block_padding, padding_byte_count);
		memcpy(to + len, padding_buffer, padding_byte_count);
		ctx->size += padding_byte_count;
	}

	dev_dbg(scc->dev, "copied %d bytes to 0x%p\n", ctx->size, to);
	pr_debug("IV:\n");
	pr_memory_display(MSG_DEBUG, scc->base + SCC_SCM_INIT_VECTOR_0, 0,
			  scc->block_size_bytes,
			     0x40 >> 3, 0);
	pr_debug("DATA:\n");
	pr_memory_display(MSG_DEBUG, to, 0, ctx->size, 0x40 >> 3, 0);

	return 0;
}

static int imx_scc_ablkcipher_next(struct imx_scc_ctx *ctx,
				   struct ablkcipher_request *ablkreq)
{
	struct imx_scc *scc = ctx->scc;
	int err;

	writel(0, scc->base + SCC_SCM_ERROR_STATUS);

	err = imx_scc_put_data(ctx, ablkreq);
	if (err)
		return err;

	dev_dbg(scc->dev, "Start encryption (0x%x/0x%x)\n",
		readl(scc->base + SCC_SCM_RED_START),
		readl(scc->base + SCC_SCM_BLACK_START));

	/* clear interrupt control registers */
	writel(SCC_SCM_INTR_CTRL_CLR_INTR,
	       scc->base + SCC_SCM_INTR_CTRL);

	writel((ctx->size / ctx->scc->block_size_bytes) - 1,
	       scc->base + SCC_SCM_LENGTH);

	dev_dbg(scc->dev, "Process %d block(s) in 0x%p\n",
		ctx->size / ctx->scc->block_size_bytes,
		(ctx->ctrl & SCC_SCM_CTRL_DECRYPT_MODE) ? scc->black_memory :
		scc->red_memory);

	writel(ctx->ctrl, scc->base + SCC_SCM_CTRL);

	return 0;
}

static int imx_scc_int(struct imx_scc_ctx *ctx)
{
	struct ablkcipher_request *ablkreq;
	struct imx_scc *scc = ctx->scc;
	uint64_t start;

	start = get_time_ns();
	while (readl(scc->base + SCC_SCM_STATUS) & SCC_SCM_STATUS_BUSY) {
		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(scc->dev, "timeout waiting for interrupt\n");
			return -ETIMEDOUT;
		}
	}

	/* clear interrupt control registers */
	writel(SCC_SCM_INTR_CTRL_CLR_INTR, scc->base + SCC_SCM_INTR_CTRL);

	ablkreq = scc->req;

	if (ablkreq)
		return imx_scc_get_data(ctx, ablkreq);

	return 0;
}

static int imx_scc_process_req(struct imx_scc_ctx *ctx,
			       struct ablkcipher_request *ablkreq)
{
	int ret = -EINPROGRESS;

	ctx->scc->req = ablkreq;

	while (ret == -EINPROGRESS) {
		ret = imx_scc_ablkcipher_next(ctx, ablkreq);
		if (ret)
			break;
		ret = imx_scc_int(ctx);
	}

	ctx->scc->req = NULL;
	ctx->scc->bytes_remaining = ctx->scc->memory_size_bytes;

	return 0;
}

static int imx_scc_des3_op(struct imx_scc_ctx *ctx,
			   struct ablkcipher_request *req)
{
	int err;

	err = imx_scc_ablkcipher_req_init(req, ctx);
	if (err)
		return err;

	return imx_scc_process_req(ctx, req);
}

int imx_scc_cbc_des_encrypt(struct ablkcipher_request *req)
{
	struct imx_scc_ctx *ctx;

	ctx = xzalloc(sizeof(*ctx));
	ctx->scc = scc_dev;

	ctx->ctrl = SCC_SCM_CTRL_START_CIPHER;
	ctx->ctrl |= SCC_SCM_CTRL_CBC_MODE;

	return imx_scc_des3_op(ctx, req);
}

int imx_scc_cbc_des_decrypt(struct ablkcipher_request *req)
{
	struct imx_scc_ctx *ctx;

	ctx = xzalloc(sizeof(*ctx));
	ctx->scc = scc_dev;

	ctx->ctrl = SCC_SCM_CTRL_START_CIPHER;
	ctx->ctrl |= SCC_SCM_CTRL_CBC_MODE;
	ctx->ctrl |= SCC_SCM_CTRL_DECRYPT_MODE;

	return imx_scc_des3_op(ctx, req);
}

static void imx_scc_hw_init(struct imx_scc *scc)
{
	int offset;

	offset = SCC_NON_RESERVED_OFFSET / scc->block_size_bytes;

	/* Fill the RED_START register */
	writel(offset, scc->base + SCC_SCM_RED_START);

	/* Fill the BLACK_START register */
	writel(offset, scc->base + SCC_SCM_BLACK_START);

	scc->red_memory = scc->base + SCC_SCM_RED_MEMORY +
			  SCC_NON_RESERVED_OFFSET;

	scc->black_memory = scc->base + SCC_SCM_BLACK_MEMORY +
			    SCC_NON_RESERVED_OFFSET;

	scc->bytes_remaining = scc->memory_size_bytes;
}

static int imx_scc_get_config(struct imx_scc *scc)
{
	int config;

	config = readl(scc->base + SCC_SCM_CFG);

	scc->block_size_bytes = config & SCC_SCM_CFG_BLOCK_SIZE_MASK;

	scc->black_ram_size_blocks = config & SCC_SCM_CFG_BLACK_SIZE_MASK;

	scc->memory_size_bytes = (scc->block_size_bytes *
				  scc->black_ram_size_blocks) -
				  SCC_NON_RESERVED_OFFSET;

	return 0;
}

static int imx_scc_get_state(struct imx_scc *scc)
{
	int status, ret;
	const char *statestr;

	status = readl(scc->base + SCC_SMN_STATUS) &
		       SCC_SMN_STATUS_STATE_MASK;

	/* If in Health Check, try to bringup to secure state */
	if (status & SCC_SMN_STATE_HEALTH_CHECK) {
		/*
		 * Write a simple algorithm to the Algorithm Sequence
		 * Checker (ASC)
		 */
		writel(0xaaaa, scc->base + SCC_SMN_SEQ_START);
		writel(0x5555, scc->base + SCC_SMN_SEQ_END);
		writel(0x5555, scc->base + SCC_SMN_SEQ_CHECK);

		status = readl(scc->base + SCC_SMN_STATUS) &
			       SCC_SMN_STATUS_STATE_MASK;
	}

	switch (status) {
	case SCC_SMN_STATE_NON_SECURE:
		statestr = "non-secure";
		ret = 0;
		break;
	case SCC_SMN_STATE_SECURE:
		statestr = "secure";
		ret = 0;
		break;
	case SCC_SMN_STATE_FAIL:
		statestr = "fail";
		ret = -EIO;
		break;
	default:
		statestr = "unknown";
		ret = -EINVAL;
		break;
	}

	dev_info(scc->dev, "starting in %s mode\n", statestr);

	return ret;
}

static int imx_scc_probe(struct device *dev)
{
	struct imx_scc *scc;
	int ret;

	scc = xzalloc(sizeof(*scc));

	scc->base = dev_request_mem_region(dev, 0);
	if (IS_ERR(scc->base))
		return PTR_ERR(scc->base);

	scc->clk = clk_get(dev, "ipg");
	if (IS_ERR(scc->clk)) {
		dev_err(dev, "Could not get ipg clock\n");
		return PTR_ERR(scc->clk);
	}

	clk_enable(scc->clk);

	/* clear error status register */

	writel(0x0, scc->base + SCC_SCM_ERROR_STATUS);

	/* clear interrupt control registers */
	writel(SCC_SCM_INTR_CTRL_CLR_INTR |
	       SCC_SCM_INTR_CTRL_MASK_INTR,
	       scc->base + SCC_SCM_INTR_CTRL);

	writel(SCC_SMN_COMMAND_CLR_INTR |
	       SCC_SMN_COMMAND_EN_INTR,
	       scc->base + SCC_SMN_COMMAND);

	scc->dev = dev;

	ret = imx_scc_get_config(scc);
	if (ret)
		goto err_out;

	ret = imx_scc_get_state(scc);

	if (ret) {
		dev_err(dev, "SCC in unusable state\n");
		goto err_out;
	}

	imx_scc_hw_init(scc);

	scc_dev = scc;

	if (IS_ENABLED(CONFIG_BLOBGEN)) {
		ret = imx_scc_blob_gen_probe(dev);
		if (ret)
			goto err_out;
	}

	return 0;

err_out:
	clk_disable(scc->clk);
	clk_put(scc->clk);
	free(scc);

	return ret;
}

static __maybe_unused struct of_device_id imx_scc_dt_ids[] = {
	{ .compatible = "fsl,imx25-scc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_scc_dt_ids);

static struct driver imx_scc_driver = {
	.name		= "mxc-scc",
	.probe		= imx_scc_probe,
	.of_compatible	= imx_scc_dt_ids,
};
device_platform_driver(imx_scc_driver);
