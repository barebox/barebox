/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __BASE64_H
#define __BASE64_H

void uuencode(char *p, const char *src, int length);
int decode_base64(char *dst, int dst_len, const char *src);
int decode_base64url(char *dst, int dst_len, const char *src);

#define BASE64_LENGTH(len)	(4 * (((len) + 2) / 3))

#endif /* __BASE64_H */
