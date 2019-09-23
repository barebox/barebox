/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Remote processor framework
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 */

#ifndef REMOTEPROC_INTERNAL_H
#define REMOTEPROC_INTERNAL_H

struct rproc;

void *rproc_da_to_va(struct rproc *rproc, u64 da, int len);

int rproc_elf_load_segments(struct rproc *rproc, const struct firmware *fw);

static inline
int rproc_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	if (rproc->ops->load)
		return rproc->ops->load(rproc, fw);

	return -EINVAL;
}

#endif /* REMOTEPROC_INTERNAL_H */
