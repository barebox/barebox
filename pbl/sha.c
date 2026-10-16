// SPDX-License-Identifier: GPL-2.0-only
/*
 * pbl_sha*() - one-shot SHA functions for the PBL, picking the best available
 * implementation.
 */

#include <common.h>
#include <crypto/sha.h>
#include <crypto/pbl-sha.h>
#include <digest.h>

static void pbl_sha256_generic(const void *buf, size_t len, u8 *out)
{
	struct sha256_state state = { };
	struct digest d = { .ctx = &state, .length = SHA256_DIGEST_SIZE };

	sha256_init(&d);
	sha256_update(&d, buf, len);
	sha256_final(&d, out);
}

void pbl_sha256(const void *buf, size_t len, u8 out[static SHA256_DIGEST_SIZE])
{
	if (!pbl_sha256_ce(buf, len, out))
		return;

	pbl_sha256_generic(buf, len, out);
}

static void pbl_sha512_generic(const void *buf, size_t len, u8 *out)
{
	struct sha512_state state = { };
	struct digest d = { .ctx = &state, .length = SHA512_DIGEST_SIZE };

	sha512_init(&d);
	sha512_update(&d, buf, len);
	sha512_final(&d, out);
}

void pbl_sha512(const void *buf, size_t len, u8 out[static SHA512_DIGEST_SIZE])
{
	pbl_sha512_generic(buf, len, out);
}

static void pbl_sha384_generic(const void *buf, size_t len, u8 *out)
{
	struct sha512_state state = { };
	struct digest d = { .ctx = &state, .length = SHA384_DIGEST_SIZE };

	sha384_init(&d);
	sha512_update(&d, buf, len);
	sha384_final(&d, out);
}

void pbl_sha384(const void *buf, size_t len, u8 out[static SHA384_DIGEST_SIZE])
{
	pbl_sha384_generic(buf, len, out);
}
