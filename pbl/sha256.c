// SPDX-License-Identifier: GPL-2.0-only
/*
 * pbl_sha256() - one-shot SHA-256 for the PBL, picking the best available
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
	pbl_sha256_generic(buf, len, out);
}
