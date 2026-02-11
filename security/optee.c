// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "optee: " fmt

#include <compressed-dtb.h>
#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/limits.h>
#include <pbl/handoff-data.h>
#include <tee/optee.h>

static u64 optee_membase = U64_MAX;

int optee_verify_header(const struct optee_header *hdr)
{
	if (IS_ERR_OR_NULL(hdr))
		return -EINVAL;

	if (hdr->magic != OPTEE_MAGIC) {
		pr_debug("Invalid header magic 0x%08x, expected 0x%08x\n",
			 hdr->magic, OPTEE_MAGIC);
		return -EINVAL;
	}

	if (hdr->version != OPTEE_VERSION_V1) {
		pr_err("Only V1 headers are supported right now\n");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_ARM32) &&
	    (hdr->arch != OPTEE_ARCH_ARM32 || hdr->init_load_addr_hi)) {
		pr_err("Wrong OP-TEE Arch for ARM v7 CPU\n");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_ARM64) && hdr->arch != OPTEE_ARCH_ARM64) {
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

void optee_handoff_overlay(void *ovl, unsigned int ovl_sz)
{
	if (!IS_ENABLED(CONFIG_OPTEE_APPLY_OVERLAY))
		return;

	if (!blob_is_fdt(ovl))
		return;

	handoff_data_add(HANDOFF_DATA_TEE_DT_OVL, ovl, ovl_sz);
}
