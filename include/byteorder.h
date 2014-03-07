#ifndef __BYTEORDER_H__
#define __BYTEORDER_H__

/*
 * The standard macros for converting between host and network byte order are
 * badly named. So ntohl converts 32 bits even on architectures where a long is
 * 64 bit wide although the 'l' suffix suggests that it's working on longs.
 *
 * So this file introduces variants that use the bitcount as suffix instead of
 * 's' or 'l'.
 */

#include <asm/byteorder.h>

#define ntoh16(x)	__be16_to_cpu(x)
#define hton16(x)	__cpu_to_be16(x)

#define ntoh32(x)	__be32_to_cpu(x)
#define hton32(x)	__cpu_to_be32(x)

#define ntoh64(x)	__be64_to_cpu(x)
#define hton64(x)	__cpu_to_be64(x)

#endif /* __BYTEORDER_H__ */
