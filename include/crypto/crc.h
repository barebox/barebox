/*
 * Common values for CRC algorithms
 */

#ifndef _CRYPTO_CRC_H
#define _CRYPTO_CRC_H

#include <linux/types.h>

#define CRC32_DIGEST_SIZE        4

struct crc32_state {
	ulong crc;
};

#endif
