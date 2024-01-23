/*
 * OP-TEE related definitions
 *
 * (C) Copyright 2016 Linaro Limited
 * Andrew F. Davis <andrew.davis@xxxxxxxxxx>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef _OPTEE_H
#define _OPTEE_H

#include <types.h>
#include <linux/errno.h>

#define OPTEE_MAGIC             0x4554504f
#define OPTEE_VERSION_V1        1
#define OPTEE_VERSION_V2        2
#define OPTEE_ARCH_ARM32        0
#define OPTEE_ARCH_ARM64        1

struct optee_header {
	uint32_t magic;
	uint8_t version;
	uint8_t arch;
	uint16_t flags;
	uint32_t init_size;
	uint32_t init_load_addr_hi;
	uint32_t init_load_addr_lo;
	uint32_t init_mem_usage;
	uint32_t paged_size;
};

int optee_verify_header (const struct optee_header *hdr);

#ifdef CONFIG_HAVE_OPTEE

void optee_set_membase(const struct optee_header *hdr);
int optee_get_membase(u64 *membase);

#else

static inline void optee_set_membase(const struct optee_header *hdr)
{
}

static inline int optee_get_membase(u64 *membase)
{
	return -ENOSYS;
}

#endif /* CONFIG_HAVE_OPTEE */

#ifdef __PBL__

int start_optee_early(void* fdt, void* tee);

#endif /* __PBL__ */

#endif /* _OPTEE_H */
