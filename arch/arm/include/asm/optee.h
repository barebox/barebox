/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ARM_OPTEE_H__
#define __ARM_OPTEE_H__

#include <linux/types.h>

struct device_node;

struct of_optee_fixup_data {
	const char *method;
	size_t shm_size;
};

int of_optee_fixup(struct device_node *root, void *fixup_data);

#endif /* __ARM_OPTEE_H__ */

