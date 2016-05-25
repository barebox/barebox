/*
 * Cryptographic API.
 *
 * CRC32.
 *
 * Derived from cryptoapi implementation, adapted for in-place
 * scatterlist interface.
 *
 * Copyright (c) Alan Smithee.
 * Copyright (c) Andrew McDonald <andrew@mcdonald.org.uk>
 * Copyright (c) Jean-Francois Dive <jef@linuxbe.org>
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
#include <linux/string.h>
#include <crc.h>
#include <asm/unaligned.h>
#include <asm/byteorder.h>

#include <crypto/crc.h>
#include <crypto/internal.h>

static int crc32_init(struct digest *desc)
{
	struct crc32_state *ctx = digest_ctx(desc);

	ctx->crc = 0;

	return 0;
}

static int crc32_update(struct digest *desc, const void *data,
			     unsigned long len)
{
	struct crc32_state *ctx = digest_ctx(desc);

	while (len) {
		int now = min((ulong)4096, len);
		ctx->crc = crc32(ctx->crc, data, now);
		len -= now;
		data += now;
	}

	return 0;
}

static int crc32_final(struct digest *desc, unsigned char *md)
{
	struct crc32_state *ctx = digest_ctx(desc);
	__be32 *dst = (__be32 *)md;

	/* Store state in digest */
	dst[0] = cpu_to_be32(ctx->crc);

	/* Wipe context */
	memset(ctx, 0, sizeof *ctx);

	return 0;
}

static struct digest_algo m = {
	.base = {
		.name		=	"crc32",
		.driver_name	=	"crc32-generic",
		.priority	=	0,
		.algo		=	HASH_ALGO_CRC32,
	},

	.init		= crc32_init,
	.update		= crc32_update,
	.final		= crc32_final,
	.digest		= digest_generic_digest,
	.verify		= digest_generic_verify,
	.length		= CRC32_DIGEST_SIZE,
	.ctx_length = sizeof(struct crc32_state),
};

static int crc32_digest_register(void)
{
	return digest_algo_register(&m);
}
device_initcall(crc32_digest_register);
