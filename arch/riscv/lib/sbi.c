// SPDX-License-Identifier: GPL-2.0-only
/*
 * SBI initialilization and all extension implementation.
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 */

#include <asm/sbi.h>
#include <linux/export.h>
#include <errno.h>
#include <init.h>

/* default SBI version is 0.1 */
unsigned long sbi_spec_version = SBI_SPEC_VERSION_DEFAULT;
EXPORT_SYMBOL(sbi_spec_version);

int sbi_err_map_linux_errno(int err)
{
	switch (err) {
	case SBI_SUCCESS:
		return 0;
	case SBI_ERR_DENIED:
		return -EPERM;
	case SBI_ERR_INVALID_PARAM:
		return -EINVAL;
	case SBI_ERR_INVALID_ADDRESS:
		return -EFAULT;
	case SBI_ERR_NOT_SUPPORTED:
	case SBI_ERR_FAILURE:
	default:
		return -ENOTSUPP;
	};
}
EXPORT_SYMBOL(sbi_err_map_linux_errno);

long __sbi_base_ecall(int fid)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_BASE, fid, 0, 0, 0, 0, 0, 0);
	if (!ret.error)
		return ret.value;
	else
		return sbi_err_map_linux_errno(ret.error);
}

static inline long sbi_get_spec_version(void)
{
	return __sbi_base_ecall(SBI_EXT_BASE_GET_SPEC_VERSION);
}

static int sbi_init(void)
{
	int ret;

	ret = sbi_get_spec_version();
	if (ret > 0)
		sbi_spec_version = ret;
	return 0;

}
core_initcall(sbi_init);
