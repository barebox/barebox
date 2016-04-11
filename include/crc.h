#ifndef __INCLUDE_CRC_H
#define __INCLUDE_CRC_H

#include <linux/types.h>

/* 16 bit CRC with polynomial x^16+x^12+x^5+1 */
extern uint16_t cyg_crc16(const unsigned char *s, int len);

uint32_t crc32(uint32_t, const void *, unsigned int);
uint32_t crc32_no_comp(uint32_t, const void *, unsigned int);
int file_crc(char *filename, unsigned long start, unsigned long size,
	     unsigned long *crc, unsigned long *total);

#endif /* __INCLUDE_CRC_H */
