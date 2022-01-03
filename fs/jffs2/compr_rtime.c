// SPDX-License-Identifier: GPL-2.0-only
/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 * Copyright © 2004-2010 David Woodhouse <dwmw2@infradead.org>
 *
 * Created by Arjan van de Ven <arjanv@redhat.com>
 *
 * Very simple lz77-ish encoder.
 *
 * Theory of operation: Both encoder and decoder have a list of "last
 * occurrences" for every possible source-value; after sending the
 * first source-byte, the second byte indicated the "run" length of
 * matches
 *
 * The algorithm is intended to only send "whole bytes", no bit-messing.
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/jffs2.h>
#include "compr.h"

static int jffs2_rtime_decompress(unsigned char *data_in,
				  unsigned char *cpage_out,
				  uint32_t srclen, uint32_t destlen)
{
	unsigned short positions[256];
	int outpos = 0;
	int pos=0;

	memset(positions,0,sizeof(positions));

	while (outpos<destlen) {
		unsigned char value;
		int backoffs;
		int repeat;

		value = data_in[pos++];
		cpage_out[outpos++] = value; /* first the verbatim copied byte */
		repeat = data_in[pos++];
		backoffs = positions[value];

		positions[value]=outpos;
		if (repeat) {
			if (backoffs + repeat >= outpos) {
				while(repeat) {
					cpage_out[outpos++] = cpage_out[backoffs++];
					repeat--;
				}
			} else {
				memcpy(&cpage_out[outpos],&cpage_out[backoffs],repeat);
				outpos+=repeat;
			}
		}
	}
	return 0;
}

static struct jffs2_compressor jffs2_rtime_comp = {
    .priority = JFFS2_RTIME_PRIORITY,
    .name = "rtime",
    .compr = JFFS2_COMPR_RTIME,
    .decompress = &jffs2_rtime_decompress,
#ifdef JFFS2_RTIME_DISABLED
    .disabled = 1,
#else
    .disabled = 0,
#endif
};

int jffs2_rtime_init(void)
{
    return jffs2_register_compressor(&jffs2_rtime_comp);
}

void jffs2_rtime_exit(void)
{
    jffs2_unregister_compressor(&jffs2_rtime_comp);
}
