/*
 * Copyright (c) 2010-2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#include <common.h>
#include <pbl.h>

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
	while (1);
}

void pbl_barebox_uncompress(void *dest, void *compressed_start, unsigned int len)
{
	decompress((void *)compressed_start,
			len,
			NULL, NULL,
			dest, NULL, errorfn);
}
