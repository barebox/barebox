#ifndef __INCLUDE_CRC_H
#define __INCLUDE_CRC_H

#include <linux/types.h>

/*
 * Implements the standard CRC ITU-T V.41:
 *   Width 16
 *   Poly  0x1021 (x^16 + x^12 + x^15 + 1)
 *   Init  0
 */
extern u16 const crc_itu_t_table[256];

extern u16 crc_itu_t(u16 crc, const u8 *buffer, size_t len);

#ifdef __PBL__
static inline u16 crc_itu_t_byte(u16 crc, const u8 data)
{
	int i;

	crc = crc ^ data << 8;
	for (i = 0; i < 8; ++i) {
		if (crc & 0x8000)
			crc = crc << 1 ^ 0x1021;
		else
			crc = crc << 1;
	}
	return crc;
}
#else
static inline u16 crc_itu_t_byte(u16 crc, const u8 data)
{
	return (crc << 8) ^ crc_itu_t_table[((crc >> 8) ^ data) & 0xff];
}
#endif

uint32_t crc32(uint32_t, const void *, unsigned int);
uint32_t crc32_no_comp(uint32_t, const void *, unsigned int);
int file_crc(char *filename, unsigned long start, unsigned long size,
	     unsigned long *crc, unsigned long *total);

#endif /* __INCLUDE_CRC_H */
