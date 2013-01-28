/*
 * Copyright (c) 2010-2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#include <common.h>
#include <pbl.h>

#define STATIC static

#ifdef CONFIG_IMAGE_COMPRESSION_LZO
#include "../../../lib/decompress_unlzo.c"
#endif

#ifdef CONFIG_IMAGE_COMPRESSION_GZIP
#include "../../../lib/decompress_inflate.c"
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
