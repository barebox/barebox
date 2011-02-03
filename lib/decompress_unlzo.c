/*
 * LZO decompressor for barebox. Code borrowed from the lzo
 * implementation by Markus Franz Xaver Johannes Oberhumer.
 *
 * Linux kernel adaptation:
 * Copyright (C) 2009
 * Albin Tonnerre, Free Electrons <albin.tonnerre@free-electrons.com>
 *
 * Original code:
 * Copyright (C) 1996-2005 Markus Franz Xaver Johannes Oberhumer
 * All Rights Reserved.
 *
 * lzop and the LZO library are free software; you can redistribute them
 * and/or modify them under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.
 * If not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Markus F.X.J. Oberhumer
 * <markus@oberhumer.com>
 * http://www.oberhumer.com/opensource/lzop/
 */

#include <common.h>
#include <malloc.h>
#include <linux/types.h>
#include <lzo.h>
#include <errno.h>
#include <fs.h>
#include <xfuncs.h>

#include <linux/compiler.h>
#include <asm/unaligned.h>

static const unsigned char lzop_magic[] = {
	0x89, 0x4c, 0x5a, 0x4f, 0x00, 0x0d, 0x0a, 0x1a, 0x0a };

#define LZO_BLOCK_SIZE        (256*1024l)
#define HEADER_HAS_FILTER      0x00000800L

static inline int parse_header(u8 *input, u8 *skip)
{
	int l;
	u8 *parse = input;
	u8 level = 0;
	u16 version;

	/* read magic: 9 first bits */
	for (l = 0; l < 9; l++) {
		if (*parse++ != lzop_magic[l])
			return 0;
	}
	/* get version (2bytes), skip library version (2),
	 * 'need to be extracted' version (2) and
	 * method (1) */
	version = get_unaligned_be16(parse);
	parse += 7;
	if (version >= 0x0940)
		level = *parse++;
	if (get_unaligned_be32(parse) & HEADER_HAS_FILTER)
		parse += 8; /* flags + filter info */
	else
		parse += 4; /* flags */

	/* skip mode and mtime_low */
	parse += 8;
	if (version >= 0x0940)
		parse += 4;	/* skip mtime_high */

	l = *parse++;
	/* don't care about the file name, and skip checksum */
	parse += l + 4;

	*skip = parse - input;
	return 1;
}

static int __unlzo(int *dest_len,
			int (*fill) (void *, unsigned int),
			int (*flush) (void *, unsigned int))
{
	u8 skip = 0, r = 0;
	u32 src_len, dst_len;
	size_t tmp;
	u8 *in_buf, *in_buf_save, *out_buf, *out_buf_save;
	int obytes_processed = 0;
	int ret = -1;

	out_buf = xmalloc(LZO_BLOCK_SIZE);
	in_buf = xmalloc(lzo1x_worst_compress(LZO_BLOCK_SIZE));

	in_buf_save = in_buf;
	out_buf_save = out_buf;

	ret = fill(in_buf, lzo1x_worst_compress(LZO_BLOCK_SIZE));
	if (ret < 0)
		goto exit_free;

	if (!parse_header(in_buf, &skip))
		return -EINVAL;

	in_buf += skip;

	for (;;) {
		/* read uncompressed block size */
		dst_len = get_unaligned_be32(in_buf);
		in_buf += 4;

		/* exit if last block */
		if (dst_len == 0)
			break;

		if (dst_len > LZO_BLOCK_SIZE) {
			printf("dest len longer than block size");
			goto exit_free;
		}

		/* read compressed block size, and skip block checksum info */
		src_len = get_unaligned_be32(in_buf);
		in_buf += 8;

		if (src_len <= 0 || src_len > dst_len) {
			printf("file corrupted");
			goto exit_free;
		}

		/* decompress */
		tmp = dst_len;

		/* When the input data is not compressed at all,
		 * lzo1x_decompress_safe will fail, so call memcpy()
		 * instead */
		if (unlikely(dst_len == src_len)) {
			if (src_len != dst_len) {
				printf("Compressed data violation");
				goto exit_free;
			}
			out_buf = in_buf;
		} else {
			r = lzo1x_decompress_safe((u8 *) in_buf, src_len,
						out_buf, &tmp);

			if (r != LZO_E_OK || dst_len != tmp) {
				printf("Compressed data violation");
				goto exit_free;
			}
		}

		ret = flush(out_buf, dst_len);
		if (ret < 0)
			goto exit_free;

		out_buf = in_buf + src_len;
		in_buf = in_buf_save;
		ret = fill(in_buf, lzo1x_worst_compress(LZO_BLOCK_SIZE));
		if (ret < 0)
			goto exit_free;

		if (ret == 0)
			in_buf = out_buf;

		out_buf = out_buf_save;

		obytes_processed += dst_len;

	}

exit_free:
	free(in_buf);
	free(out_buf);

	*dest_len = obytes_processed;
	return 0;
}

static int in_fd;
static int out_fd;

static int unlzo_fill(void *buf, unsigned int len)
{
	return read(in_fd, buf, len);
}

static int unlzo_flush(void *buf, unsigned int len)
{
	return write(out_fd, buf, len);
}

int unlzo(int _in_fd, int _out_fd, int *dest_len)
{
	in_fd = _in_fd;
	out_fd = _out_fd;
	return __unlzo(dest_len, unlzo_fill, unlzo_flush);
}
