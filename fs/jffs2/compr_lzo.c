// SPDX-License-Identifier: GPL-2.0-only
/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2007 Nokia Corporation. All rights reserved.
 * Copyright © 2004-2010 David Woodhouse <dwmw2@infradead.org>
 *
 * Created by Richard Purdie <rpurdie@openedhand.com>
 */

#include <common.h>
#include <lzo.h>
#include <linux/kernel.h>
#include "compr.h"

static int jffs2_lzo_decompress(unsigned char *data_in, unsigned char *cpage_out,
				 uint32_t srclen, uint32_t destlen)
{
	size_t dl = destlen;
	int ret;

	ret = lzo1x_decompress_safe(data_in, srclen, cpage_out, &dl);

	if (ret != LZO_E_OK || dl != destlen)
		return -1;

	return 0;
}

static struct jffs2_compressor jffs2_lzo_comp = {
	.priority = JFFS2_LZO_PRIORITY,
	.name = "lzo",
	.compr = JFFS2_COMPR_LZO,
	.decompress = &jffs2_lzo_decompress,
	.disabled = 0,
};

int __init jffs2_lzo_init(void)
{
	int ret;

	ret = jffs2_register_compressor(&jffs2_lzo_comp);

	return ret;
}

void jffs2_lzo_exit(void)
{
	jffs2_unregister_compressor(&jffs2_lzo_comp);
}
