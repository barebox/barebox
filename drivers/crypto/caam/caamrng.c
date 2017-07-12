/*
 * caam - Freescale FSL CAAM support for hw_random
 *
 * Copyright (C) 2011-2015 Freescale Semiconductor, Inc.
 *
 * Based on caamalg.c crypto API driver.
 *
 * relationship between job descriptors to shared descriptors:
 *
 * ---------------                     --------------
 * | JobDesc #0  |-------------------->| ShareDesc  |
 * | *(buffer 0) |      |------------->| (generate) |
 * ---------------      |              | (move)     |
 *                      |              | (store)    |
 * ---------------      |              --------------
 * | JobDesc #1  |------|
 * | *(buffer 1) |
 * ---------------
 *
 * A job desc looks like this:
 *
 * ---------------------
 * | Header            |
 * | ShareDesc Pointer |
 * | SEQ_OUT_PTR       |
 * | (output buffer)   |
 * ---------------------
 *
 * The SharedDesc never changes, and each job descriptor points to one of two
 * buffers for each device, from which the data will be copied into the
 * requested destination
 */
#include <common.h>
#include <dma.h>
#include <driver.h>
#include <init.h>
#include <linux/spinlock.h>
#include <linux/hw_random.h>

#include "regs.h"
#include "intern.h"
#include "desc_constr.h"
#include "jr.h"
#include "error.h"

/*
 * Maximum buffer size: maximum number of random, cache-aligned bytes that
 * will be generated and moved to seq out ptr (extlen not allowed)
 */
#define RN_BUF_SIZE			32767

/* length of descriptors */
#define DESC_JOB_O_LEN			(CAAM_CMD_SZ * 2 + CAAM_PTR_SZ * 2)
#define DESC_RNG_LEN			(10 * CAAM_CMD_SZ)

/* Buffer, its dma address and lock */
struct buf_data {
	u8 *buf;
	dma_addr_t addr;
	u32 hw_desc[DESC_JOB_O_LEN];
#define BUF_NOT_EMPTY 0
#define BUF_EMPTY 1
#define BUF_PENDING 2  /* Empty, but with job pending --don't submit another */
	int empty;
};

/* rng per-device context */
struct caam_rng_ctx {
	struct device_d *jrdev;
	dma_addr_t sh_desc_dma;
	u32 sh_desc[DESC_RNG_LEN];
	unsigned int cur_buf_idx;
	int current_buf;
	struct buf_data bufs[2];
	struct hwrng rng;
};

static struct caam_rng_ctx *rng_ctx;

static void rng_done(struct device_d *jrdev, u32 *desc, u32 err, void *context)
{
	struct buf_data *bd;

	bd = (struct buf_data *)((char *)desc -
	      offsetof(struct buf_data, hw_desc));

	if (err)
		caam_jr_strstatus(jrdev, err);

	bd->empty = BUF_NOT_EMPTY;

	/* Buffer refilled, invalidate cache */
	dma_sync_single_for_cpu(bd->addr, RN_BUF_SIZE, DMA_FROM_DEVICE);

#ifdef DEBUG
	print_hex_dump(KERN_ERR, "rng refreshed buf@: ",
		       DUMP_PREFIX_OFFSET, 16, 4, bd->buf, RN_BUF_SIZE, 1);
#endif
}

static inline int submit_job(struct caam_rng_ctx *ctx, int to_current)
{
	struct buf_data *bd = &ctx->bufs[!(to_current ^ ctx->current_buf)];
	struct device_d *jrdev = ctx->jrdev;
	u32 *desc = bd->hw_desc;
	int err;

	dev_dbg(jrdev, "submitting job %d\n", !(to_current ^ ctx->current_buf));

	dma_sync_single_for_device((unsigned long)desc, desc_bytes(desc),
				   DMA_TO_DEVICE);

	err = caam_jr_enqueue(jrdev, desc, rng_done, ctx);
	if (!err)
		bd->empty += 1; /* note if pending */

	return err;
}

static int caam_read(struct hwrng *rng, void *data, size_t max, bool wait)
{
	struct caam_rng_ctx *ctx = container_of(rng, struct caam_rng_ctx, rng);
	struct buf_data *bd = &ctx->bufs[ctx->current_buf];
	int next_buf_idx, copied_idx;
	int err;

	if (bd->empty) {
		/* try to submit job if there wasn't one */
		if (bd->empty == BUF_EMPTY) {
			err = submit_job(ctx, 1);
			/* if can't submit job, can't even wait */
			if (err)
				return 0;
		}
		/* no immediate data, so exit if not waiting */
		if (!wait)
			return 0;
	}

	next_buf_idx = ctx->cur_buf_idx + max;
	dev_dbg(ctx->jrdev, "%s: start reading at buffer %d, idx %d\n",
		 __func__, ctx->current_buf, ctx->cur_buf_idx);

	/* if enough data in current buffer */
	if (next_buf_idx < RN_BUF_SIZE) {
		memcpy(data, bd->buf + ctx->cur_buf_idx, max);
		ctx->cur_buf_idx = next_buf_idx;
		return max;
	}

	/* else, copy what's left... */
	copied_idx = RN_BUF_SIZE - ctx->cur_buf_idx;
	memcpy(data, bd->buf + ctx->cur_buf_idx, copied_idx);
	ctx->cur_buf_idx = 0;
	bd->empty = BUF_EMPTY;

	/* ...refill... */
	err = submit_job(ctx, 1);
	if (err)
		return err;

	/* and use next buffer */
	ctx->current_buf = !ctx->current_buf;
	dev_dbg(ctx->jrdev, "switched to buffer %d\n", ctx->current_buf);

	/* since there already is some data read, don't wait */
	return copied_idx + caam_read(rng, data + copied_idx,
				      max - copied_idx, false);
}

static inline int rng_create_sh_desc(struct caam_rng_ctx *ctx)
{
	u32 *desc = ctx->sh_desc;

	init_sh_desc(desc, HDR_SHARE_SERIAL);

	/* Propagate errors from shared to job descriptor */
	append_cmd(desc, SET_OK_NO_PROP_ERRORS | CMD_LOAD);

	/* Generate random bytes */
	append_operation(desc, OP_ALG_ALGSEL_RNG | OP_TYPE_CLASS1_ALG);

	/* Store bytes */
	append_seq_fifo_store(desc, RN_BUF_SIZE, FIFOST_TYPE_RNGSTORE);

	ctx->sh_desc_dma = (dma_addr_t)desc;

	dma_sync_single_for_device((unsigned long)desc, desc_bytes(desc),
				   DMA_TO_DEVICE);
#ifdef DEBUG
	print_hex_dump(KERN_ERR, "rng shdesc@: ", DUMP_PREFIX_OFFSET, 16, 4,
		       desc, desc_bytes(desc), 1);
#endif
	return 0;
}

static inline int rng_create_job_desc(struct caam_rng_ctx *ctx, int buf_id)
{
	struct buf_data *bd = &ctx->bufs[buf_id];
	u32 *desc = bd->hw_desc;
	int sh_len = desc_len(ctx->sh_desc);

	init_job_desc_shared(desc, ctx->sh_desc_dma, sh_len, HDR_SHARE_DEFER |
			     HDR_REVERSE);

	append_seq_out_ptr_intlen(desc, bd->addr, RN_BUF_SIZE, 0);
#ifdef DEBUG
	print_hex_dump(KERN_ERR, "rng job desc@: ", DUMP_PREFIX_OFFSET, 16, 4,
		       desc, desc_bytes(desc), 1);
#endif
	return 0;
}

static int caam_init_buf(struct caam_rng_ctx *ctx, int buf_id)
{
	struct buf_data *bd = &ctx->bufs[buf_id];
	int err;

	bd->buf = dma_alloc_coherent(RN_BUF_SIZE, &bd->addr);

	err = rng_create_job_desc(ctx, buf_id);
	if (err)
		return err;

	bd->empty = BUF_EMPTY;
	return submit_job(ctx, buf_id == ctx->current_buf);
}

static int caam_init_rng(struct caam_rng_ctx *ctx, struct device_d *jrdev)
{
	int err;

	ctx->jrdev = jrdev;

	err = rng_create_sh_desc(ctx);
	if (err)
		return err;

	ctx->current_buf = 0;
	ctx->cur_buf_idx = 0;

	err = caam_init_buf(ctx, 0);
	if (err)
		return err;

	err = caam_init_buf(ctx, 1);
	if (err)
		return err;

	return 0;
}

int caam_rng_probe(struct device_d *dev, struct device_d *jrdev)
{
	int err;

	rng_ctx = xzalloc(sizeof(*rng_ctx));

	err = caam_init_rng(rng_ctx, jrdev);
	if (err)
		return err;

	rng_ctx->rng.name = dev->name;
	rng_ctx->rng.read = caam_read;

	err = hwrng_register(dev, &rng_ctx->rng);
	if (err) {
		dev_err(dev, "rng-caam registering failed (%d)\n", err);
		return err;
	}

	dev_info(dev, "registering rng-caam\n");

	return 0;
}
