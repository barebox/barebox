// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "optee: " fmt

#include <tee/optee.h>
#include <printk.h>
#include <asm-generic/errno.h>

int optee_verify_header(struct optee_header *hdr)
{
	if (hdr->magic != OPTEE_MAGIC) {
		pr_err("Invalid header magic 0x%08x, expected 0x%08x\n",
			   hdr->magic, OPTEE_MAGIC);
		return -EINVAL;
	}

	if (hdr->arch != OPTEE_ARCH_ARM32 || hdr->init_load_addr_hi) {
		pr_err("Only 32bit supported\n");
		return -EINVAL;
	}

	return 0;
}
