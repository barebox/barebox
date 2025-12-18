/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __INCLUDE_CRC_H
#define __INCLUDE_CRC_H

#include <linux/types.h>

/*
 * Implements the standard CRC ITU-T V.41:
 *   Width 16
 *   Poly  0x1021 (x^16 + x^12 + x^15 + 1)
 *   Init  0
 */
extern u16 crc_itu_t(u16 crc, const u8 *buffer, size_t len);

uint32_t crc32(uint32_t, const void *, unsigned int);
uint32_t crc32_be(uint32_t, const void *, unsigned int);
uint32_t crc32_no_comp(uint32_t, const void *, unsigned int);
int file_crc(char *filename, unsigned long start, unsigned long size,
	     unsigned long *crc, unsigned long *total);

uint32_t __pi_crc32(uint32_t, const void *, unsigned int);

#endif /* __INCLUDE_CRC_H */
