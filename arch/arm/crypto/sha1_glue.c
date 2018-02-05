/*
 * Cryptographic API.
 * Glue code for the SHA1 Secure Hash Algorithm assembler implementation
 *
 * This file is based on sha1_generic.c and sha1_ssse3_glue.c
 *
 * Copyright (c) Alan Smithee.
 * Copyright (c) Andrew McDonald <andrew@mcdonald.org.uk>
 * Copyright (c) Jean-Francois Dive <jef@linuxbe.org>
 * Copyright (c) Mathias Krause <minipli@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <common.h>
#include <digest.h>
#include <init.h>
#include <crypto/sha.h>
#include <crypto/internal.h>
#include <asm/byteorder.h>

void sha1_block_data_order(u32 *digest,
		const unsigned char *data, unsigned int rounds);


static int sha1_init(struct digest *desc)
{
	struct sha1_state *sctx = digest_ctx(desc);

	*sctx = (struct sha1_state){
		.state = { SHA1_H0, SHA1_H1, SHA1_H2, SHA1_H3, SHA1_H4 },
	};

	return 0;
}


static int __sha1_update(struct sha1_state *sctx, const u8 *data,
			 unsigned int len, unsigned int partial)
{
	unsigned int done = 0;

	sctx->count += len;

	if (partial) {
		done = SHA1_BLOCK_SIZE - partial;
		memcpy(sctx->buffer + partial, data, done);
		sha1_block_data_order(sctx->state, sctx->buffer, 1);
	}

	if (len - done >= SHA1_BLOCK_SIZE) {
		const unsigned int rounds = (len - done) / SHA1_BLOCK_SIZE;
		sha1_block_data_order(sctx->state, data + done, rounds);
		done += rounds * SHA1_BLOCK_SIZE;
	}

	memcpy(sctx->buffer, data + done, len - done);
	return 0;
}


int sha1_update_arm(struct digest *desc, const void *data,
			     unsigned long len)
{
	struct sha1_state *sctx = digest_ctx(desc);
	unsigned int partial = sctx->count % SHA1_BLOCK_SIZE;
	int res;

	/* Handle the fast case right here */
	if (partial + len < SHA1_BLOCK_SIZE) {
		sctx->count += len;
		memcpy(sctx->buffer + partial, data, len);
		return 0;
	}
	res = __sha1_update(sctx, data, len, partial);
	return res;
}
EXPORT_SYMBOL_GPL(sha1_update_arm);


/* Add padding and return the message digest. */
static int sha1_final(struct digest *desc, u8 *out)
{
	struct sha1_state *sctx = digest_ctx(desc);
	unsigned int i, index, padlen;
	__be32 *dst = (__be32 *)out;
	__be64 bits;
	static const u8 padding[SHA1_BLOCK_SIZE] = { 0x80, };

	bits = cpu_to_be64(sctx->count << 3);

	/* Pad out to 56 mod 64 and append length */
	index = sctx->count % SHA1_BLOCK_SIZE;
	padlen = (index < 56) ? (56 - index) : ((SHA1_BLOCK_SIZE+56) - index);
	/* We need to fill a whole block for __sha1_update() */
	if (padlen <= 56) {
		sctx->count += padlen;
		memcpy(sctx->buffer + index, padding, padlen);
	} else {
		__sha1_update(sctx, padding, padlen, index);
	}
	__sha1_update(sctx, (const u8 *)&bits, sizeof(bits), 56);

	/* Store state in digest */
	for (i = 0; i < 5; i++)
		dst[i] = cpu_to_be32(sctx->state[i]);

	/* Wipe context */
	memset(sctx, 0, sizeof(*sctx));
	return 0;
}

static struct digest_algo m = {
	.base = {
		.name		=	"sha1",
		.driver_name	=	"sha1-asm",
		.priority	=	150,
		.algo		=	HASH_ALGO_SHA1,
	},

	.init	=	sha1_init,
	.update	=	sha1_update_arm,
	.final	=	sha1_final,
	.digest	=	digest_generic_digest,
	.verify	=	digest_generic_verify,
	.length	=	SHA1_DIGEST_SIZE,
	.ctx_length =	sizeof(struct sha1_state),
};

static int sha1_mod_init(void)
{
	return digest_algo_register(&m);
}
coredevice_initcall(sha1_mod_init);
