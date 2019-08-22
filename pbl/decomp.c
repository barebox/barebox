/*
 * Copyright (c) 2010-2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#include <common.h>
#include <crypto/sha.h>
#include <crypto/pbl-sha.h>
#include <digest.h>
#include <asm/sections.h>
#include <pbl.h>
#include <debug_ll.h>

#define STATIC static

#ifdef CONFIG_IMAGE_COMPRESSION_LZ4
#include "../../../lib/decompress_unlz4.c"
#endif

#ifdef CONFIG_IMAGE_COMPRESSION_LZO
#include "../../../lib/decompress_unlzo.c"
#endif

#ifdef CONFIG_IMAGE_COMPRESSION_GZIP
#include "../../../lib/decompress_inflate.c"
#endif

#ifdef CONFIG_IMAGE_COMPRESSION_XZKERN
#include "../../../lib/decompress_unxz.c"
#endif

#ifdef CONFIG_IMAGE_COMPRESSION_NONE
STATIC int decompress(u8 *input, int in_len,
				int (*fill) (void *, unsigned int),
				int (*flush) (void *, unsigned int),
				u8 *output, int *posp,
				void (*error) (char *x))
{
	memcpy(output, input, in_len);
	return 0;
}
#endif

static void noinline errorfn(char *error)
{
	puts_ll("ERROR: ");
	puts_ll(error);
	puts_ll("\nHANG\n");
	while (1);
}

extern unsigned char sha_sum[];
extern unsigned char sha_sum_end[];

static int pbl_barebox_verify(void *compressed_start, unsigned int len, void *hash,
			      unsigned int hash_len)
{
	struct sha256_state sha_state = { 0 };
	struct digest d = { .ctx = &sha_state };
	char computed_hash[SHA256_DIGEST_SIZE];
	int i;
	char *char_hash = hash;

	if (hash_len != SHA256_DIGEST_SIZE)
		return -1;

	sha256_init(&d);
	sha256_update(&d, compressed_start, len);
	sha256_final(&d, computed_hash);
	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		putc_ll('C');
		putc_ll('H');
		putc_ll('\n');
		for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
			puthex_ll(computed_hash[i]);
			putc_ll('\n');
		}
		putc_ll('I');
		putc_ll('H');
		putc_ll('\n');
		for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
			puthex_ll(char_hash[i]);
			putc_ll('\n');
		}
	}

	return memcmp(hash, computed_hash, SHA256_DIGEST_SIZE);
}

void pbl_barebox_uncompress(void *dest, void *compressed_start, unsigned int len)
{
	uint32_t pbl_hash_len;
	void *pbl_hash_start, *pbl_hash_end;

	if (IS_ENABLED(CONFIG_PBL_VERIFY_PIGGY)) {
		pbl_hash_start = sha_sum;
		pbl_hash_end = sha_sum_end;
		pbl_hash_len = pbl_hash_end - pbl_hash_start;
		if (pbl_barebox_verify(compressed_start, len, pbl_hash_start,
				       pbl_hash_len) != 0) {
			putc_ll('!');
			panic("hash mismatch, refusing to decompress");
		}
	}

	decompress((void *)compressed_start,
			len,
			NULL, NULL,
			dest, NULL, errorfn);
}
