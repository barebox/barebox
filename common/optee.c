// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "optee: " fmt

#include <tee/optee.h>
#include <linux/printk.h>
#include <linux/errno.h>

static u64 optee_membase = U64_MAX;

int optee_verify_header(const struct optee_header *hdr)
{
	if (!hdr)
		return -EINVAL;

	if (hdr->magic != OPTEE_MAGIC) {
		pr_err("Invalid header magic 0x%08x, expected 0x%08x\n",
			   hdr->magic, OPTEE_MAGIC);
		return -EINVAL;
	}

	if (hdr->version != OPTEE_VERSION_V1) {
		pr_err("Only V1 headers are supported right now\n");
		return -EINVAL;
	}

	if (IS_ENABLED(CPU_V7) &&
	    (hdr->arch != OPTEE_ARCH_ARM32 || hdr->init_load_addr_hi)) {
		pr_err("Wrong OP-TEE Arch for ARM v7 CPU\n");
		return -EINVAL;
	}

	if (IS_ENABLED(CPU_V8) && hdr->arch != OPTEE_ARCH_ARM64) {
		pr_err("Wrong OP-TEE Arch for ARM v8 CPU\n");
		return -EINVAL;
	}

	return 0;
}

int optee_get_membase(u64 *membase)
{
	if (optee_membase == U64_MAX)
		return -EINVAL;

	*membase = optee_membase;

	return 0;
}

void optee_set_membase(const struct optee_header *hdr)
{
	int ret;

	ret = optee_verify_header(hdr);
	if (ret)
		return;

	optee_membase = (u64)hdr->init_load_addr_hi << 32;
	optee_membase |= hdr->init_load_addr_lo;
}
