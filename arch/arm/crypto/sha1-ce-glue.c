// SPDX-License-Identifier: GPL-2.0-only
/*
 * sha1-ce-glue.c - SHA-1 secure hash using ARMv8 Crypto Extensions
 *
 * Copyright (C) 2014 - 2017 Linaro Ltd <ard.biesheuvel@linaro.org>
 */

#include <common.h>
#include <digest.h>
#include <init.h>
#include <crypto/sha.h>
#include <crypto/sha1_base.h>
#include <crypto/internal.h>
#include <linux/linkage.h>
#include <asm/byteorder.h>
#include <asm/neon.h>

MODULE_DESCRIPTION("SHA1 secure hash using ARMv8 Crypto Extensions");
MODULE_AUTHOR("Ard Biesheuvel <ard.biesheuvel@linaro.org>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_CRYPTO("sha1");

struct sha1_ce_state {
	struct sha1_state	sst;
	u32			finalize;
};

extern const u32 sha1_ce_offsetof_count;
extern const u32 sha1_ce_offsetof_finalize;

asmlinkage int sha1_ce_transform(struct sha1_ce_state *sst, u8 const *src,
				 int blocks);

static void __sha1_ce_transform(struct sha1_state *sst, u8 const *src,
				int blocks)
{
	while (blocks) {
		int rem;

		kernel_neon_begin();
		rem = sha1_ce_transform(container_of(sst, struct sha1_ce_state,
						     sst), src, blocks);
		kernel_neon_end();
		src += (blocks - rem) * SHA1_BLOCK_SIZE;
		blocks = rem;
	}
}

const u32 sha1_ce_offsetof_count = offsetof(struct sha1_ce_state, sst.count);
const u32 sha1_ce_offsetof_finalize = offsetof(struct sha1_ce_state, finalize);

static int sha1_ce_update(struct digest *desc, const void *data,
			  unsigned long len)
{
	struct sha1_ce_state *sctx = digest_ctx(desc);

	sctx->finalize = 0;
	sha1_base_do_update(desc, data, len, __sha1_ce_transform);

	return 0;
}

static int sha1_ce_final(struct digest *desc, u8 *out)
{
	struct sha1_ce_state *sctx = digest_ctx(desc);

	sctx->finalize = 0;
	sha1_base_do_finalize(desc, __sha1_ce_transform);
	return sha1_base_finish(desc, out);
}

static struct digest_algo m = {
	.base = {
		.name		=	"sha1",
		.driver_name	=	"sha1-ce",
		.priority	=	200,
		.algo		=	HASH_ALGO_SHA1,
	},

	.init	=	sha1_base_init,
	.update	=	sha1_ce_update,
	.final	=	sha1_ce_final,
	.digest	=	digest_generic_digest,
	.verify	=	digest_generic_verify,
	.length	=	SHA1_DIGEST_SIZE,
	.ctx_length =	sizeof(struct sha1_ce_state),
};

static int sha1_ce_mod_init(void)
{
	return digest_algo_register(&m);
}
coredevice_initcall(sha1_ce_mod_init);
