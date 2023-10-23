/*
 * Code based on busybox-1.23.2
 *
 * Copyright 2003, Glenn McGrath
 * Copyright 2006, Rob Landley <rob@landley.net>
 * Copyright 2010, Denys Vlasenko
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include <common.h>
#include <base64.h>

/* Conversion table.  for base 64 */
static const char uuenc_tbl_base64[65 + 1] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
	'=' /* termination character */,
	'\0' /* needed for uudecode.c only */
};

static char base64_trchr(char ch, bool url)
{
	if (!url)
		return ch;

	switch (ch) {
	case '+':
		return '-';
	case '/':
		return '_';
	case '-':
		return '+';
	case '_':
		return '/';
	default:
		return ch;
	}
}

/*
 * Encode bytes at S of length LENGTH to uuencode or base64 format and place it
 * to STORE.  STORE will be 0-terminated, and must point to a writable
 * buffer of at least 1+BASE64_LENGTH(length) bytes.
 * where BASE64_LENGTH(len) = (4 * ((LENGTH + 2) / 3))
 */
void uuencode(char *p, const char *src, int length)
{
	const unsigned char *s = src;
	const char *tbl = uuenc_tbl_base64;

	/* Transform the 3x8 bits to 4x6 bits */
	while (length > 0) {
		unsigned s1, s2;

		/* Are s[1], s[2] valid or should be assumed 0? */
		s1 = s2 = 0;
		length -= 3; /* can be >=0, -1, -2 */
		if (length >= -1) {
			s1 = s[1];
			if (length >= 0)
				s2 = s[2];
		}
		*p++ = tbl[s[0] >> 2];
		*p++ = tbl[((s[0] & 3) << 4) + (s1 >> 4)];
		*p++ = tbl[((s1 & 0xf) << 2) + (s2 >> 6)];
		*p++ = tbl[s2 & 0x3f];
		s += 3;
	}
	/* Zero-terminate */
	*p = '\0';
	/* If length is -2 or -1, pad last char or two */
	while (length) {
		*--p = tbl[64];
		length++;
	}
}
EXPORT_SYMBOL(uuencode);

/*
 * Decode base64 encoded string. Stops on '\0'.
 *
 */
static int __decode_base64(char *p_dst, int dst_len, const char *src, bool url)
{
	const char *src_tail;
	char *dst = p_dst;
	int length = 0;
	bool end_reached = false;

	while (dst_len > 0 && !end_reached) {
		unsigned char six_bit[4];
		int count = 0;

		/* Fetch up to four 6-bit values */
		src_tail = src;
		while (count < 4) {
			const char *table_ptr;
			int ch;

			/*
			 * Get next _valid_ character.
			 * uuenc_tbl_base64[] contains this string:
			 *  0         1         2         3         4         5         6
			 *  01234567890123456789012345678901234567890123456789012345678901234
			 * "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
			 */
			do {
				ch = *src;
				if (ch == '\0') {
					/*
					 * Example:
					 * If we decode "QUJD <NUL>", we want
					 * to return ptr to NUL, not to ' ',
					 * because we did fully decode
					 * the string (to "ABC").
					 */
					if (count == 0) {
						src_tail = src;
					} else if (url) {
						end_reached = true;
						goto out;
					}

					goto ret;
				}
				src++;
				table_ptr = strchr(uuenc_tbl_base64, base64_trchr(ch, url));
			} while (!table_ptr && !url);

			if (!table_ptr) {
				end_reached = true;
				goto out;
			}

			/* Convert encoded character to decimal */
			ch = table_ptr - uuenc_tbl_base64;

			/* ch is 64 if char was '=', otherwise 0..63 */
			if (ch == 64)
				break;

			six_bit[count] = ch;
			count++;
		}
out:

		/*
		 * Transform 6-bit values to 8-bit ones.
		 * count can be < 4 when we decode the tail:
		 * "eQ==" -> "y", not "y NUL NUL".
		 * Note that (count > 1) is always true,
		 * "x===" encoding is not valid:
		 * even a single zero byte encodes as "AA==".
		 * However, with current logic we come here with count == 1
		 * when we decode "==" tail.
		 */
		if (count > 1)
			*dst++ = six_bit[0] << 2 | six_bit[1] >> 4;
		if (count > 2)
			*dst++ = six_bit[1] << 4 | six_bit[2] >> 2;
		if (count > 3)
			*dst++ = six_bit[2] << 6 | six_bit[3];
		/*
		 * Note that if we decode "AA==" and ate first '=',
		 * we just decoded one char (count == 2) and now we'll
		 * do the loop once more to decode second '='.
		 */
		dst_len -= count-1;
		/* last character was "=" */
		if (count != 0)
			length += count - 1;
	}
ret:
	p_dst = dst;

	return length;
}

/*
 * Decode base64 encoded string. Stops on '\0'.
 *
 */
int decode_base64(char *p_dst, int dst_len, const char *src)
{
	return __decode_base64(p_dst, dst_len, src, false);
}
EXPORT_SYMBOL(decode_base64);

/*
 * Decode base64url encoded string. Stops on '\0'.
 *
 */
int decode_base64url(char *p_dst, int dst_len, const char *src)
{
	return __decode_base64(p_dst, dst_len, src, true);
}
EXPORT_SYMBOL(decode_base64url);
