#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#include <linux/types.h>
#include <string.h>
#include <asm/byteorder.h>

#define _B(n)	((unsigned long long)((uint8_t *)&x)[n])

#define fdt32_to_cpu(x)	be32_to_cpu(x)
#define cpu_to_fdt32(x) cpu_to_be32(x)

static inline uint64_t fdt64_to_cpu(uint64_t x)
{
	return (_B(0) << 56) | (_B(1) << 48) | (_B(2) << 40) | (_B(3) << 32)
		| (_B(4) << 24) | (_B(5) << 16) | (_B(6) << 8) | _B(7);
}
#define cpu_to_fdt64(x) fdt64_to_cpu(x)
#undef _B

#endif /* _LIBFDT_ENV_H */
