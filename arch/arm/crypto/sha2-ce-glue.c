// SPDX-License-Identifier: GPL-2.0-only
/*
 * sha2-ce-glue.c - SHA-224/SHA-256 using ARMv8 Crypto Extensions
 *
 * Copyright (C) 2014 - 2017 Linaro Ltd <ard.biesheuvel@linaro.org>
 */

#include <common.h>
#include <digest.h>
#include <init.h>
#include <crypto/sha.h>
#include <crypto/sha256_base.h>
#include <crypto/internal.h>
#include <linux/linkage.h>
#include <asm/byteorder.h>
#include <asm/neon.h>

#include <asm/neon.h>

MODULE_DESCRIPTION("SHA-224/SHA-256 secure hash using ARMv8 Crypto Extensions");
MODULE_AUTHOR("Ard Biesheuvel <ard.biesheuvel@linaro.org>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_CRYPTO("sha224");
MODULE_ALIAS_CRYPTO("sha256");

struct sha256_ce_state {
	struct sha256_state	sst;
	u32			finalize;
};

extern const u32 sha256_ce_offsetof_count;
extern const u32 sha256_ce_offsetof_finalize;

asmlinkage int sha2_ce_transform(struct sha256_ce_state *sst, u8 const *src,
				 int blocks);

static void __sha2_ce_transform(struct sha256_state *sst, u8 const *src,
				int blocks)
{
	while (blocks) {
		int rem;

		kernel_neon_begin();
		rem = sha2_ce_transform(container_of(sst, struct sha256_ce_state,
						     sst), src, blocks);
		kernel_neon_end();
		src += (blocks - rem) * SHA256_BLOCK_SIZE;
		blocks = rem;
	}
}

const u32 sha256_ce_offsetof_count = offsetof(struct sha256_ce_state,
					      sst.count);
const u32 sha256_ce_offsetof_finalize = offsetof(struct sha256_ce_state,
						 finalize);

static int sha256_ce_update(struct digest *desc, const void *data,
			    unsigned long len)
{
	struct sha256_ce_state *sctx = digest_ctx(desc);

	sctx->finalize = 0;
	sha256_base_do_update(desc, data, len, __sha2_ce_transform);

	return 0;
}

static int sha256_ce_final(struct digest *desc, u8 *out)
{
	struct sha256_ce_state *sctx = digest_ctx(desc);

	sctx->finalize = 0;
	sha256_base_do_finalize(desc, __sha2_ce_transform);
	return sha256_base_finish(desc, out);
}

static struct digest_algo sha224 = {
	.base = {
		.name		=	"sha224",
		.driver_name	=	"sha224-ce",
		.priority	=	200,
		.algo		=	HASH_ALGO_SHA224,
	},

	.length	=	SHA224_DIGEST_SIZE,
	.init	=	sha224_base_init,
	.update	=	sha256_ce_update,
	.final	=	sha256_ce_final,
	.digest	=	digest_generic_digest,
	.verify	=	digest_generic_verify,
	.ctx_length =	sizeof(struct sha256_ce_state),
};

static int sha224_ce_digest_register(void)
{
	return digest_algo_register(&sha224);
}
coredevice_initcall(sha224_ce_digest_register);

static struct digest_algo sha256 = {
	.base = {
		.name		=	"sha256",
		.driver_name	=	"sha256-ce",
		.priority	=	200,
		.algo		=	HASH_ALGO_SHA256,
	},

	.length	=	SHA256_DIGEST_SIZE,
	.init	=	sha256_base_init,
	.update	=	sha256_ce_update,
	.final	=	sha256_ce_final,
	.digest	=	digest_generic_digest,
	.verify	=	digest_generic_verify,
	.ctx_length =	sizeof(struct sha256_ce_state),
};

static int sha256_ce_digest_register(void)
{
	return digest_algo_register(&sha256);
}
coredevice_initcall(sha256_ce_digest_register);
