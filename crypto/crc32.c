/*
 * This file is derived from crc32.c from the zlib-1.1.3 distribution
 * by Jean-loup Gailly and Mark Adler.
 */

/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifdef __BAREBOX__		/* Shut down "ANSI does not permit..." warnings */
#include <common.h>
#include <xfuncs.h>
#include <fs.h>
#include <crc.h>
#include <fcntl.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <errno.h>
#define STATIC
#else
#define STATIC static inline
#endif

static uint32_t crc_table[sizeof(uint32_t) * 256];

/*
  Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The table is simply the CRC of all possible eight bit values.  This is all
  the information needed to generate CRC's on data a byte at a time for all
  combinations of CRC register values and incoming bytes.
*/
static void make_crc_table(void)
{
	uint32_t c;
	int n, k;
	uint32_t poly;		/* polynomial exclusive-or pattern */
	/* terms of polynomial defining this crc (except x^32): */
	static const char p[] = { 0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26 };

	if (crc_table[1])
		return;

	/* make exclusive-or pattern from polynomial (0xedb88320L) */
	poly = 0;
	for (n = 0; n < sizeof(p) / sizeof(char); n++)
		poly |= 1U << (31 - p[n]);

	for (n = 0; n < 256; n++) {
		c = (uint32_t) n;
		for (k = 0; k < 8; k++)
			c = c & 1 ? poly ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
}

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

STATIC uint32_t crc32(uint32_t crc, const void *_buf, unsigned int len)
{
	const unsigned char *buf = _buf;

	make_crc_table();

	crc = crc ^ 0xffffffffL;
	while (len >= 8) {
		DO8(buf);
		len -= 8;
	}
	if (len)
		do {
			DO1(buf);
		} while (--len);
	return crc ^ 0xffffffffL;
}

#ifdef __BAREBOX__
EXPORT_SYMBOL(crc32);
#endif

/* No ones complement version. JFFS2 (and other things ?)
 * don't use ones compliment in their CRC calculations.
 */
STATIC uint32_t crc32_no_comp(uint32_t crc, const void *_buf, unsigned int len)
{
	const unsigned char *buf = _buf;

	make_crc_table();

	while (len >= 8) {
		DO8(buf);
		len -= 8;
	}
	if (len)
		do {
			DO1(buf);
		} while (--len);

	return crc;
}

STATIC uint32_t crc32_be(uint32_t crc, const void *_buf, unsigned int len)
{
	const unsigned char *buf = _buf;
	int i;

	while (len--) {
		crc ^= *buf++ << 24;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x80000000) ? 0x04c11db7 : 0);
	}
	return crc;
}

STATIC int file_crc(char *filename, ulong start, ulong size, ulong * crc,
		    ulong * total)
{
	int fd, now;
	int ret = 0;
	char *buf;

	*total = 0;
	*crc = 0;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("open %s: %s\n", filename, strerror(errno));
		return fd;
	}

	if (start > 0) {
		off_t lseek_ret;
		errno = 0;
		lseek_ret = lseek(fd, start, SEEK_SET);
		if (lseek_ret == (off_t)-1 && errno) {
			perror("lseek");
			ret = -1;
			goto out;
		}
	}

	buf = xmalloc(4096);

	while (size) {
		now = min((ulong)4096, size);
		now = read(fd, buf, now);
		if (now < 0) {
			ret = now;
			perror("read");
			goto out_free;
		}
		if (!now)
			break;
		*crc = crc32(*crc, buf, now);
		size -= now;
		*total += now;
	}

out_free:
	free(buf);
out:
	close(fd);

	return ret;
}

#ifdef __BAREBOX__
EXPORT_SYMBOL(file_crc);
#endif
