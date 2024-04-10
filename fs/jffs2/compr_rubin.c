// SPDX-License-Identifier: GPL-2.0-only
/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 * Copyright © 2004-2010 David Woodhouse <dwmw2@infradead.org>
 *
 * Created by Arjan van de Ven <arjanv@redhat.com>
 */

#define pr_fmt(fmt) "jffs2: " fmt

#include <linux/string.h>
#include <linux/types.h>
#include <linux/jffs2.h>
#include "compr.h"


#define RUBIN_REG_SIZE   16
#define UPPER_BIT_RUBIN    (((long) 1)<<(RUBIN_REG_SIZE-1))
#define LOWER_BITS_RUBIN   ((((long) 1)<<(RUBIN_REG_SIZE-1))-1)


#define BIT_DIVIDER_MIPS 1043
static int bits_mips[8] = { 277, 249, 290, 267, 229, 341, 212, 241};

struct pushpull {
	unsigned char *buf;
	unsigned int buflen;
	unsigned int ofs;
	unsigned int reserve;
};

struct rubin_state {
	unsigned long p;
	unsigned long q;
	unsigned long rec_q;
	long bit_number;
	struct pushpull pp;
	int bit_divider;
	int bits[8];
};

static inline void init_pushpull(struct pushpull *pp, char *buf,
				 unsigned buflen, unsigned ofs,
				 unsigned reserve)
{
	pp->buf = buf;
	pp->buflen = buflen;
	pp->ofs = ofs;
	pp->reserve = reserve;
}

static inline int pushbit(struct pushpull *pp, int bit, int use_reserved)
{
	if (pp->ofs >= pp->buflen - (use_reserved?0:pp->reserve))
		return -ENOSPC;

	if (bit)
		pp->buf[pp->ofs >> 3] |= (1<<(7-(pp->ofs & 7)));
	else
		pp->buf[pp->ofs >> 3] &= ~(1<<(7-(pp->ofs & 7)));

	pp->ofs++;

	return 0;
}

static inline int pushedbits(struct pushpull *pp)
{
	return pp->ofs;
}

static inline int pullbit(struct pushpull *pp)
{
	int bit;

	bit = (pp->buf[pp->ofs >> 3] >> (7-(pp->ofs & 7))) & 1;

	pp->ofs++;
	return bit;
}


static void init_rubin(struct rubin_state *rs, int div, int *bits)
{
	int c;

	rs->q = 0;
	rs->p = (long) (2 * UPPER_BIT_RUBIN);
	rs->bit_number = (long) 0;
	rs->bit_divider = div;

	for (c=0; c<8; c++)
		rs->bits[c] = bits[c];
}

static void init_decode(struct rubin_state *rs, int div, int *bits)
{
	init_rubin(rs, div, bits);

	/* behalve lower */
	rs->rec_q = 0;

	for (rs->bit_number = 0; rs->bit_number++ < RUBIN_REG_SIZE;
	     rs->rec_q = rs->rec_q * 2 + (long) (pullbit(&rs->pp)))
		;
}

static void __do_decode(struct rubin_state *rs, unsigned long p,
			unsigned long q)
{
	register unsigned long lower_bits_rubin = LOWER_BITS_RUBIN;
	unsigned long rec_q;
	int c, bits = 0;

	/*
	 * First, work out how many bits we need from the input stream.
	 * Note that we have already done the initial check on this
	 * loop prior to calling this function.
	 */
	do {
		bits++;
		q &= lower_bits_rubin;
		q <<= 1;
		p <<= 1;
	} while ((q >= UPPER_BIT_RUBIN) || ((p + q) <= UPPER_BIT_RUBIN));

	rs->p = p;
	rs->q = q;

	rs->bit_number += bits;

	/*
	 * Now get the bits.  We really want this to be "get n bits".
	 */
	rec_q = rs->rec_q;
	do {
		c = pullbit(&rs->pp);
		rec_q &= lower_bits_rubin;
		rec_q <<= 1;
		rec_q += c;
	} while (--bits);
	rs->rec_q = rec_q;
}

static int decode(struct rubin_state *rs, long A, long B)
{
	unsigned long p = rs->p, q = rs->q;
	long i0, threshold;
	int symbol;

	if (q >= UPPER_BIT_RUBIN || ((p + q) <= UPPER_BIT_RUBIN))
		__do_decode(rs, p, q);

	i0 = A * rs->p / (A + B);
	if (i0 <= 0)
		i0 = 1;

	if (i0 >= rs->p)
		i0 = rs->p - 1;

	threshold = rs->q + i0;
	symbol = rs->rec_q >= threshold;
	if (rs->rec_q >= threshold) {
		rs->q += i0;
		i0 = rs->p - i0;
	}

	rs->p = i0;

	return symbol;
}

static int in_byte(struct rubin_state *rs)
{
	int i, result = 0, bit_divider = rs->bit_divider;

	for (i = 0; i < 8; i++)
		result |= decode(rs, bit_divider - rs->bits[i],
				 rs->bits[i]) << i;

	return result;
}

static void rubin_do_decompress(int bit_divider, int *bits,
				unsigned char *cdata_in,
				unsigned char *page_out, uint32_t srclen,
				uint32_t destlen)
{
	int outpos = 0;
	struct rubin_state rs;

	init_pushpull(&rs.pp, cdata_in, srclen, 0, 0);
	init_decode(&rs, bit_divider, bits);

	while (outpos < destlen)
		page_out[outpos++] = in_byte(&rs);
}


static int jffs2_rubinmips_decompress(unsigned char *data_in,
				      unsigned char *cpage_out,
				      uint32_t sourcelen, uint32_t dstlen)
{
	rubin_do_decompress(BIT_DIVIDER_MIPS, bits_mips, data_in,
			    cpage_out, sourcelen, dstlen);
	return 0;
}

static int jffs2_dynrubin_decompress(unsigned char *data_in,
				     unsigned char *cpage_out,
				     uint32_t sourcelen, uint32_t dstlen)
{
	int bits[8];
	int c;

	for (c=0; c<8; c++)
		bits[c] = data_in[c];

	rubin_do_decompress(256, bits, data_in+8, cpage_out, sourcelen-8,
			    dstlen);
	return 0;
}

static struct jffs2_compressor jffs2_rubinmips_comp = {
	.priority = JFFS2_RUBINMIPS_PRIORITY,
	.name = "rubinmips",
	.compr = JFFS2_COMPR_DYNRUBIN,
	.decompress = &jffs2_rubinmips_decompress,
#ifdef JFFS2_RUBINMIPS_DISABLED
	.disabled = 1,
#else
	.disabled = 0,
#endif
};

int jffs2_rubinmips_init(void)
{
	return jffs2_register_compressor(&jffs2_rubinmips_comp);
}

void jffs2_rubinmips_exit(void)
{
	jffs2_unregister_compressor(&jffs2_rubinmips_comp);
}

static struct jffs2_compressor jffs2_dynrubin_comp = {
	.priority = JFFS2_DYNRUBIN_PRIORITY,
	.name = "dynrubin",
	.compr = JFFS2_COMPR_RUBINMIPS,
	.decompress = &jffs2_dynrubin_decompress,
#ifdef JFFS2_DYNRUBIN_DISABLED
	.disabled = 1,
#else
	.disabled = 0,
#endif
};

int jffs2_dynrubin_init(void)
{
	return jffs2_register_compressor(&jffs2_dynrubin_comp);
}

void jffs2_dynrubin_exit(void)
{
	jffs2_unregister_compressor(&jffs2_dynrubin_comp);
}
