/*
 * uncompress.c - uncompress files
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <uncompress.h>
#include <bunzip2.h>
#include <gunzip.h>
#include <lzo.h>
#include <linux/xz.h>
#include <linux/decompress/unlz4.h>
#include <linux/decompress/unzstd.h>
#include <errno.h>
#include <filetype.h>
#include <malloc.h>
#include <fs.h>
#include <libfile.h>

static void *uncompress_buf;
static unsigned long uncompress_size;

void uncompress_err_stdout(char *x)
{
	printf("%s\n", x);
}

static long (*uncompress_fill_fn)(void*, unsigned long);

static long uncompress_fill(void *buf, unsigned long len)
{
	long total = 0;

	if (uncompress_size) {
		int now = min(len, uncompress_size);

		memcpy(buf, uncompress_buf, now);
		uncompress_size -= now;
		len -= now;
		total = now;
		buf += now;
	}

	if (len) {
		int ret = uncompress_fill_fn(buf, len);
		if (ret < 0)
			return ret;
		total += ret;
	}

	return total;
}

int uncompress(unsigned char *inbuf, long len,
	   long(*fill)(void*, unsigned long),
	   long(*flush)(void*, unsigned long),
	   unsigned char *output,
	   long *pos,
	   void(*error_fn)(char *x))
{
	enum filetype ft;
	int (*compfn)(unsigned char *inbuf, long len,
            long(*fill)(void*, unsigned long),
            long(*flush)(void*, unsigned long),
            unsigned char *output,
            long *pos,
            void(*error)(char *x));
	int ret;
	char *err;

	if (inbuf) {
		ft = file_detect_type(inbuf, len);
		uncompress_buf = NULL;
		uncompress_size = 0;
	} else {
		if (!fill)
			return -EINVAL;

		uncompress_fill_fn = fill;
		uncompress_buf = xzalloc(32);
		uncompress_size = 32;

		ret = fill(uncompress_buf, 32);
		if (ret < 0)
			goto err;

		ft = file_detect_type(uncompress_buf, 32);
	}

	pr_debug("Filetype detected: %s\n", file_type_to_string(ft));

	switch (ft) {
#ifdef CONFIG_BZLIB
	case filetype_bzip2:
		compfn = bunzip2;
		break;
#endif
#ifdef CONFIG_ZLIB
	case filetype_gzip:
		compfn = gunzip;
		break;
#endif
#ifdef CONFIG_LZO_DECOMPRESS
	case filetype_lzo_compressed:
		compfn = decompress_unlzo;
		break;
#endif
#ifdef CONFIG_LZ4_DECOMPRESS
	case filetype_lz4_compressed:
		compfn = decompress_unlz4;
		break;
#endif
#ifdef CONFIG_XZ_DECOMPRESS
	case filetype_xz_compressed:
		compfn = decompress_unxz;
		break;
#endif
#ifdef CONFIG_ZSTD_DECOMPRESS
	case filetype_zstd_compressed:
		compfn = unzstd;
		break;
#endif
	default:
		err = basprintf("unsupported compression filetype \"%s\"",
				  file_type_to_string(ft));
		error_fn(err);
		free(err);
		ret = -ENOSYS;
		goto err;
	}

	ret = compfn(inbuf, len, fill ? uncompress_fill : NULL,
			flush, output, pos, error_fn);
err:
	free(uncompress_buf);

	return ret;
}

static int uncompress_infd, uncompress_outfd;

static long fill_fd(void *buf, unsigned long len)
{
	return read_full(uncompress_infd, buf, len);
}

static long flush_fd(void *buf, unsigned long len)
{
	return write(uncompress_outfd, buf, len);
}

int uncompress_fd_to_fd(int infd, int outfd,
	   void(*error_fn)(char *x))
{
	uncompress_infd = infd;
	uncompress_outfd = outfd;

	return uncompress(NULL, 0,
	   fill_fd,
	   flush_fd,
	   NULL,
	   NULL,
	   error_fn);
}

int uncompress_fd_to_buf(int infd, void *output,
		void(*error_fn)(char *x))
{
	uncompress_infd = infd;

	return uncompress(NULL, 0, fill_fd, NULL, output, NULL, error_fn);
}

int uncompress_buf_to_fd(const void *input, size_t input_len,
			 int outfd, void(*error_fn)(char *x))
{
	uncompress_outfd = outfd;

	return uncompress((void *)input, input_len, NULL, flush_fd,
			  NULL, NULL, error_fn);
}

ssize_t uncompress_buf_to_buf(const void *input, size_t input_len,
			      void **buf, void(*error_fn)(char *x))
{
	size_t size;
	int fd, ret;
	void *p;

	fd = open("/tmp", O_TMPFILE | O_RDWR);
	if (fd < 0)
		return -ENODEV;

	ret = uncompress_buf_to_fd(input, input_len, fd, error_fn);
	if (ret)
		goto close_fd;

	p = read_fd(fd, &size);
	if (p)
		*buf = p;
	else
		ret = -errno;

close_fd:
	close(fd);

	return ret ?: size;
}
