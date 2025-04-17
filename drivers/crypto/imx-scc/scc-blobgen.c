// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2016 Pengutronix, Steffen Trumtrar <kernel@pengutronix.de>
 */

#include <common.h>
#include <dma.h>
#include <digest.h>
#include <driver.h>
#include <init.h>
#include <blobgen.h>
#include <stdlib.h>
#include <crypto.h>
#include <crypto/sha.h>

#include "scc.h"

#define MAX_IVLEN		BLOCKSIZE_BYTES

static struct digest *sha256;

static int sha256sum(uint8_t *src, uint8_t *dst, unsigned int size)
{
	if (!sha256)
		sha256 = digest_alloc("sha256");

	if (!sha256) {
		pr_err("Unable to allocate sha256 digest\n");
		return -EINVAL;
	}

	return digest_digest(sha256, src, size, dst);
}

static int imx_scc_blob_encrypt(struct blobgen *bg, const char *modifier,
				const void *plain, int plainsize, void *blob,
				int *blobsize)
{
	char *s;
	int bufsiz;
	struct ablkcipher_request req = {};
	uint8_t iv[MAX_IVLEN];
	uint8_t hash[SHA256_DIGEST_SIZE];
	int ret;

	bufsiz = ALIGN(plainsize + KEYMOD_LENGTH, 8);

	s = malloc(bufsiz + SHA256_DIGEST_SIZE);
	if (!s)
		return -ENOMEM;

	memset(s, 0, bufsiz);

	strncpy(s, modifier, KEYMOD_LENGTH);
	memcpy(s + KEYMOD_LENGTH, plain, plainsize);

	ret = sha256sum(s, hash, bufsiz);
	if (ret)
		goto out;

	memcpy(s + bufsiz, hash, SHA256_DIGEST_SIZE);

	bufsiz += SHA256_DIGEST_SIZE;

	req.info = iv;
	req.src = s;
	req.dst = blob;
	req.nbytes = bufsiz;

	get_crypto_bytes(req.info, MAX_IVLEN);

	ret = imx_scc_cbc_des_encrypt(&req);
	if (ret)
		goto out;

	memcpy(blob + bufsiz, req.info, MAX_IVLEN);
	*blobsize = bufsiz + MAX_IVLEN;

out:
	free(s);

	return ret;
}

static int imx_scc_blob_decrypt(struct blobgen *bg, const char *modifier,
				const void *blob, int blobsize, void **plain,
				int *plainsize)
{
	struct ablkcipher_request req = {};
	uint8_t iv[MAX_IVLEN];
	uint8_t hash[SHA256_DIGEST_SIZE];
	int ret;
	uint8_t *data;
	int ciphersize = blobsize - MAX_IVLEN;

	if (blobsize <= MAX_IVLEN + SHA256_DIGEST_SIZE + KEYMOD_LENGTH)
		return -EINVAL;

	data = malloc(ciphersize);
	if (!data)
		return -ENOMEM;

	req.info = iv;
	req.nbytes = ciphersize;
	req.src = (void *)blob;
	req.dst = data;

	memcpy(req.info, blob + req.nbytes, MAX_IVLEN);

	ret = imx_scc_cbc_des_decrypt(&req);
	if (ret)
		goto out;

	ret = sha256sum(data, hash, ciphersize - SHA256_DIGEST_SIZE);
	if (ret)
		goto out;

	if (memcmp(data + ciphersize - SHA256_DIGEST_SIZE, hash,
	    SHA256_DIGEST_SIZE)) {
		pr_err("%s: Corrupted SHA256 digest. Can't continue.\n",
		       bg->dev.name);
		pr_err("%s: Calculated hash:\n", bg->dev.name);
		memory_display(hash, 0, SHA256_DIGEST_SIZE, 1, 0);
		pr_err("%s: Received hash:\n", bg->dev.name);
		memory_display(data + ciphersize - SHA256_DIGEST_SIZE,
			       0, SHA256_DIGEST_SIZE, 1, 0);

		ret = -EILSEQ;
		goto out;
	}

	*plainsize = ciphersize - SHA256_DIGEST_SIZE - KEYMOD_LENGTH;
	*plain = xmemdup(data + KEYMOD_LENGTH, *plainsize);
out:
	free(data);

	return ret;
}

int imx_scc_blob_gen_probe(struct device *dev)
{
	struct blobgen *bg;
	int ret;

	bg = xzalloc(sizeof(*bg));

	bg->max_payload_size = MAX_BLOB_LEN - MAX_IVLEN -
				SHA256_DIGEST_SIZE - KEYMOD_LENGTH;
	bg->encrypt = imx_scc_blob_encrypt;
	bg->decrypt = imx_scc_blob_decrypt;

	ret = blob_gen_register(dev, bg);
	if (ret)
		free(bg);

	return ret;
}
