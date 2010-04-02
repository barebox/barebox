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

static inline int parse_header(int in_fd)
{
	u8 l;
	u16 version;
	int ret;
	unsigned char buf[256]; /* maximum filename length + 1 */

	/* read magic (9), library version (2), 'need to be extracted'
	 * version (2) and method (1)
	 */
	ret = read(in_fd, buf, 9);
	if (ret < 0)
		return ret;

	/* check magic */
	for (l = 0; l < 9; l++) {
		if (buf[l] != lzop_magic[l])
			return -EINVAL;
	}

	ret = read(in_fd, buf, 4); /* version, lib_version */
	if (ret < 0)
		return ret;
	version = get_unaligned_be16(buf);

	if (version >= 0x0940) {
		ret = read(in_fd, buf, 2); /* version to extract */
		if (ret < 0)
			return ret;
	}

	ret = read(in_fd, buf, 1); /* method */
	if (ret < 0)
		return ret;

	if (version >= 0x0940)
		read(in_fd, buf, 1); /* level */

	ret = read(in_fd, buf, 4); /* flags */
	if (ret < 0)
		return ret;

	if (get_unaligned_be32(buf) & HEADER_HAS_FILTER) {
		ret = read(in_fd, buf, 4); /* skip filter info */
		if (ret < 0)
			return ret;
	}

	/* skip mode and mtime_low */
	ret = read(in_fd, buf, 8);
	if (ret < 0)
		return ret;

	if (version >= 0x0940) {
		ret = read(in_fd, buf, 4); /* skip mtime_high */
		if (ret < 0)
			return ret;
	}

	ret = read(in_fd, &l, 1);
	if (ret < 0)
		return ret;
	/* don't care about the file name, and skip checksum */
	ret = read(in_fd, buf, l + 4);
	if (ret < 0)
		return ret;

	return 0;
}

int unlzo(int in_fd, int out_fd, int *dest_len)
{
	u8 r = 0;
	u32 src_len, dst_len;
	size_t tmp;
	u8 *in_buf, *out_buf;
	int obytes_processed = 0;
	unsigned char buf[8];
	int ret;

	if (parse_header(in_fd))
		return -EINVAL;

	out_buf = xmalloc(LZO_BLOCK_SIZE);
	in_buf = xmalloc(lzo1x_worst_compress(LZO_BLOCK_SIZE));

	for (;;) {
		/* read uncompressed block size */
		ret = read(in_fd, buf, 4);
		if (ret < 0)
			goto exit_free;
		dst_len = get_unaligned_be32(buf);

		/* exit if last block */
		if (dst_len == 0)
			break;

		if (dst_len > LZO_BLOCK_SIZE) {
			printf("dest len longer than block size");
			goto exit_free;
		}

		/* read compressed block size, and skip block checksum info */
		ret = read(in_fd, buf, 8);
		if (ret < 0)
			goto exit_free;

		src_len = get_unaligned_be32(buf);

		if (src_len <= 0 || src_len > dst_len) {
			printf("file corrupted");
			goto exit_free;
		}
		ret = read(in_fd, in_buf, src_len);
		if (ret < 0)
			goto exit_free;

		/* decompress */
		tmp = dst_len;
		if (src_len < dst_len) {
			r = lzo1x_decompress_safe((u8 *) in_buf, src_len,
						out_buf, &tmp);
			if (r != LZO_E_OK || dst_len != tmp) {
				printf("Compressed data violation");
				goto exit_free;
			}
			ret = write(out_fd, out_buf, dst_len);
			if (ret < 0)
				goto exit_free;
		} else {
			if (src_len != dst_len) {
				printf("Compressed data violation");
				goto exit_free;
			}
			ret = write(out_fd, in_buf, dst_len);
			if (ret < 0)
				goto exit_free;
		}

		obytes_processed += dst_len;
	}

exit_free:
	free(in_buf);
	free(out_buf);

	*dest_len = obytes_processed;
	return 0;
}

