// SPDX-License-Identifier: GPL-2.0-only
/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 * Copyright © 2004-2010 David Woodhouse <dwmw2@infradead.org>
 *
 * Created by David Woodhouse <dwmw2@infradead.org>
 */
#define pr_fmt(fmt) "jffs2: " fmt
#include <common.h>
#include <linux/kernel.h>
#include <linux/zlib.h>
#include <linux/zutil.h>
#include "nodelist.h"
#include "compr.h"

	/* Plan: call deflate() with avail_in == *sourcelen,
		avail_out = *dstlen - 12 and flush == Z_FINISH.
		If it doesn't manage to finish,	call it again with
		avail_in == 0 and avail_out set to the remaining 12
		bytes for it to clean up.
	   Q: Is 12 bytes sufficient?
	*/
#define STREAM_END_SPACE 12

static struct z_stream_s inf_strm;

static int jffs2_zlib_decompress(unsigned char *data_in,
				 unsigned char *cpage_out,
				 uint32_t srclen, uint32_t destlen)
{
	int ret;
	int wbits = MAX_WBITS;

	inf_strm.next_in = data_in;
	inf_strm.avail_in = srclen;
	inf_strm.total_in = 0;

	inf_strm.next_out = cpage_out;
	inf_strm.avail_out = destlen;
	inf_strm.total_out = 0;

	/* If it's deflate, and it's got no preset dictionary, then
	   we can tell zlib to skip the adler32 check. */
	if (srclen > 2 && !(data_in[1] & PRESET_DICT) &&
	    ((data_in[0] & 0x0f) == Z_DEFLATED) &&
	    !(((data_in[0]<<8) + data_in[1]) % 31)) {

		jffs2_dbg(2, "inflate skipping adler32\n");
		wbits = -((data_in[0] >> 4) + 8);
		inf_strm.next_in += 2;
		inf_strm.avail_in -= 2;
	} else {
		/* Let this remain D1 for now -- it should never happen */
		jffs2_dbg(1, "inflate not skipping adler32\n");
	}


	if (Z_OK != zlib_inflateInit2(&inf_strm, wbits)) {
		pr_warn("inflateInit failed\n");
		return 1;
	}

	while((ret = zlib_inflate(&inf_strm, Z_FINISH)) == Z_OK)
		;
	if (ret != Z_STREAM_END) {
		pr_notice("inflate returned %d\n", ret);
	}
	zlib_inflateEnd(&inf_strm);
	return 0;
}

static struct jffs2_compressor jffs2_zlib_comp = {
    .priority = JFFS2_ZLIB_PRIORITY,
    .name = "zlib",
    .compr = JFFS2_COMPR_ZLIB,
    .decompress = &jffs2_zlib_decompress,
#ifdef JFFS2_ZLIB_DISABLED
    .disabled = 1,
#else
    .disabled = 0,
#endif
};

int __init jffs2_zlib_init(void)
{
    int ret;

    inf_strm.workspace = vmalloc(zlib_inflate_workspacesize());
    jffs2_dbg(1, "Allocated %d bytes for inflate workspace\n",
	      zlib_inflate_workspacesize());

    ret = jffs2_register_compressor(&jffs2_zlib_comp);

    return ret;
}

void jffs2_zlib_exit(void)
{
	jffs2_unregister_compressor(&jffs2_zlib_comp);
	vfree(inf_strm.workspace);
}
