/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Remote Processor Framework
 *
 * Copyright(c) 2011 Texas Instruments, Inc.
 * Copyright(c) 2011 Google, Inc.
 */

#ifndef REMOTEPROC_H
#define REMOTEPROC_H

#include <firmware.h>

struct resource_table {
	u32 ver;
	u32 num;
	u32 reserved[2];
	u32 offset[0];
} __packed;

struct firmware {
	size_t size;
	const u8 *data;
};

struct rproc;

struct rproc_ops {
	int (*start)(struct rproc *rproc);
	int (*stop)(struct rproc *rproc);
	void * (*da_to_va)(struct rproc *rproc, u64 da, int len);
	int (*load)(struct rproc *rproc, const struct firmware *fw);
};

struct rproc {
	struct firmware_handler fh;
	const char *name;
	void *priv;
	struct rproc_ops *ops;
	struct device_d dev;
	int index;

	void *fw_buf;
	size_t fw_buf_ofs;
};

struct rproc *rproc_alloc(struct device_d *dev, const char *name,
			  const struct rproc_ops *ops, int len);
int rproc_add(struct rproc *rproc);

#endif /* REMOTEPROC_H */
