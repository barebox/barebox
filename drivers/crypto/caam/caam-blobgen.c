// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 Pengutronix, Steffen Trumtrar <kernel@pengutronix.de>
 */
#include <common.h>
#include <asm/io.h>
#include <base64.h>
#include <blobgen.h>
#include <crypto.h>
#include <dma.h>
#include <driver.h>
#include <init.h>
#include <fs.h>
#include <fcntl.h>
#include "intern.h"
#include "desc.h"
#include "desc_constr.h"
#include "error.h"
#include "jr.h"

/*
 * Upon completion, desc points to a buffer containing a CAAM job
 * descriptor which encapsulates data into an externally-storable
 * blob.
 */
#define INITIAL_DESCSZ		16
/* 32 bytes key blob + 16 bytes HMAC identifier */
#define BLOB_OVERHEAD		(32 + 16)
#define KEYMOD_LENGTH		16
#define RED_BLOB_LENGTH		64
#define MAX_BLOB_LEN		4096
#define DESC_LEN		64

struct blob_job_result {
        int err;
};

struct blob_priv {
	struct blobgen bg;
	u32 desc[DESC_LEN];
	dma_addr_t dma_modifier;
	dma_addr_t dma_plaintext;
	dma_addr_t dma_ciphertext;
};

static struct blob_priv *to_blob_priv(struct blobgen *bg)
{
	return container_of(bg, struct blob_priv, bg);
}

static void jr_jobdesc_blob_decap(struct blob_priv *ctx, u8 modlen, u16 input_size)
{
	u32 *desc = ctx->desc;
	u16 in_sz;
	u16 out_sz;

	in_sz = input_size;
	out_sz = input_size - BLOB_OVERHEAD;

	init_job_desc(desc, 0);
	/*
	 * The key modifier can be used to differentiate specific data.
	 * Or to prevent replay attacks.
	 */
	append_key(desc, ctx->dma_modifier, modlen, CLASS_2);
	append_seq_in_ptr(desc, ctx->dma_ciphertext, in_sz, 0);
	append_seq_out_ptr(desc, ctx->dma_plaintext, out_sz, 0);
	append_operation(desc, OP_TYPE_DECAP_PROTOCOL | OP_PCLID_BLOB);
}

static void jr_jobdesc_blob_encap(struct blob_priv *ctx, u8 modlen, u16 input_size)
{
	u32 *desc = ctx->desc;
	u16 in_sz;
	u16 out_sz;

	in_sz = input_size;
	out_sz = input_size + BLOB_OVERHEAD;

	init_job_desc(desc, 0);
	/*
	 * The key modifier can be used to differentiate specific data.
	 * Or to prevent replay attacks.
	 */
	append_key(desc, ctx->dma_modifier, modlen, CLASS_2);
	append_seq_in_ptr(desc, ctx->dma_plaintext, in_sz, 0);
	append_seq_out_ptr(desc, ctx->dma_ciphertext, out_sz, 0);
	append_operation(desc, OP_TYPE_ENCAP_PROTOCOL | OP_PCLID_BLOB);
}

static void blob_job_done(struct device *dev, u32 *desc, u32 err, void *arg)
{
	struct blob_job_result *res = arg;

	if (!res)
		return;

	if (err)
		caam_jr_strstatus(dev, err);

	res->err = err;
}

static int caam_blob_decrypt(struct blobgen *bg, const char *modifier,
			     const void *blob, int blobsize, void **plain,
			     int *plainsize)
{
	struct blob_priv *ctx = to_blob_priv(bg);
	struct device *jrdev = bg->dev.parent;
	struct blob_job_result testres;
	int modifier_len = strlen(modifier);
	u32 *desc = ctx->desc;
	int ret;

	if (blobsize <= BLOB_OVERHEAD)
		return -EINVAL;

	*plainsize = blobsize - BLOB_OVERHEAD;

	*plain = dma_alloc(*plainsize);
	if (!*plain)
		return -ENOMEM;

	memset(desc, 0, DESC_LEN);

	ctx->dma_modifier =   (dma_addr_t)modifier;
	ctx->dma_plaintext =  (dma_addr_t)*plain;
	ctx->dma_ciphertext = (dma_addr_t)blob;

	jr_jobdesc_blob_decap(ctx, modifier_len, blobsize);

	dma_sync_single_for_device(jrdev, (unsigned long)desc, desc_bytes(desc),
				   DMA_TO_DEVICE);

	dma_sync_single_for_device(jrdev, (unsigned long)modifier, modifier_len,
				   DMA_TO_DEVICE);
	dma_sync_single_for_device(jrdev, (unsigned long)*plain, *plainsize,
				   DMA_FROM_DEVICE);
	dma_sync_single_for_device(jrdev, (unsigned long)blob, blobsize,
				   DMA_TO_DEVICE);

	testres.err = 0;

	ret = caam_jr_enqueue(jrdev, desc, blob_job_done, &testres);
	if (ret)
		dev_err(jrdev, "decryption error\n");

	ret = testres.err;

	dma_sync_single_for_cpu(jrdev, (unsigned long)modifier, modifier_len,
				DMA_TO_DEVICE);
	dma_sync_single_for_cpu(jrdev, (unsigned long)*plain, *plainsize,
				DMA_FROM_DEVICE);
	dma_sync_single_for_cpu(jrdev, (unsigned long)blob, blobsize,
				DMA_TO_DEVICE);

	return ret;
}

static int caam_blob_encrypt(struct blobgen *bg, const char *modifier,
			     const void *plain, int plainsize, void *blob,
			     int *blobsize)
{
	struct blob_priv *ctx = to_blob_priv(bg);
	struct device *jrdev = bg->dev.parent;
	struct blob_job_result testres;
	int modifier_len = strlen(modifier);
	u32 *desc = ctx->desc;
	int ret;

	*blobsize = plainsize + BLOB_OVERHEAD;

	memset(desc, 0, DESC_LEN);

	ctx->dma_modifier =   (dma_addr_t)modifier;
	ctx->dma_plaintext =  (dma_addr_t)plain;
	ctx->dma_ciphertext = (dma_addr_t)blob;

	jr_jobdesc_blob_encap(ctx, modifier_len, plainsize);

	dma_sync_single_for_device(jrdev, (unsigned long)desc, desc_bytes(desc),
				   DMA_TO_DEVICE);

	dma_sync_single_for_device(jrdev, (unsigned long)modifier, modifier_len,
				   DMA_TO_DEVICE);
	dma_sync_single_for_device(jrdev, (unsigned long)plain, plainsize,
				   DMA_TO_DEVICE);
	dma_sync_single_for_device(jrdev, (unsigned long)blob, *blobsize,
				   DMA_FROM_DEVICE);

	testres.err = 0;

	ret = caam_jr_enqueue(jrdev, desc, blob_job_done, &testres);
	if (ret)
		dev_err(jrdev, "encryption error\n");

	ret = testres.err;

	dma_sync_single_for_cpu(jrdev, (unsigned long)modifier, modifier_len,
				DMA_TO_DEVICE);
	dma_sync_single_for_cpu(jrdev, (unsigned long)plain, plainsize,
				DMA_TO_DEVICE);
	dma_sync_single_for_cpu(jrdev, (unsigned long)blob, *blobsize,
				DMA_FROM_DEVICE);

	return ret;
}

int caam_blob_gen_probe(struct device *dev, struct device *jrdev)
{
	struct blob_priv *ctx;
	struct blobgen *bg;
	int ret;

	ctx = xzalloc(sizeof(*ctx));
	bg = &ctx->bg;
	bg->max_payload_size = MAX_BLOB_LEN - BLOB_OVERHEAD;
	bg->encrypt = caam_blob_encrypt;
	bg->decrypt = caam_blob_decrypt;

	ret = blob_gen_register(jrdev, bg);
	if (ret)
		free(ctx);

	return ret;
}
