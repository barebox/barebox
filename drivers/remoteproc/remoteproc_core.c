// SPDX-License-Identifier: GPL-2.0-only
/*
 * Remote Processor Framework
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 * Mark Grosen <mgrosen@ti.com>
 * Fernando Guzman Lugo <fernando.lugo@ti.com>
 * Suman Anna <s-anna@ti.com>
 * Robert Tivy <rtivy@ti.com>
 * Armando Uribe De Leon <x0095078@ti.com>
 */

#include <common.h>
#include <firmware.h>
#include <linux/remoteproc.h>

#include "remoteproc_internal.h"

void *rproc_da_to_va(struct rproc *rproc, u64 da, int len)
{
	void *ptr;

	if (rproc->ops->da_to_va) {
		ptr = rproc->ops->da_to_va(rproc, da, len);
		if (ptr)
			return ptr;
	}

	return NULL;
}

static int rproc_start(struct rproc *rproc, const struct firmware *fw)
{
	struct device_d *dev = &rproc->dev;
	int ret;

	/* load the ELF segments to memory */
	ret = rproc_load_segments(rproc, fw);
	if (ret) {
		dev_err(dev, "Failed to load program segments: %d\n", ret);
		return ret;
	}

	/* power up the remote processor */
	ret = rproc->ops->start(rproc);
	if (ret) {
		dev_err(dev, "can't start rproc %s: %d\n", rproc->name, ret);
		return ret;
	}

	dev_info(dev, "remote processor %s is now up\n", rproc->name);

	return 0;
}

static int rproc_firmware_start(struct firmware_handler *fh)
{
	struct rproc *rproc = container_of(fh, struct rproc, fh);

	rproc->fw_buf = kzalloc((4096 * 1024), GFP_KERNEL);
	rproc->fw_buf_ofs = 0;
	return 0;
}

static int rproc_firmware_write_buf(struct firmware_handler *fh, const void *buf,
		size_t size)
{
	struct rproc *rproc = container_of(fh, struct rproc, fh);

	if (rproc->fw_buf_ofs + size > (4096 * 1024)) {
		return -ENOMEM;
	}

	memcpy(rproc->fw_buf + rproc->fw_buf_ofs, buf, size);
	rproc->fw_buf_ofs += size;

	return 0;
}

static int rproc_firmware_finish(struct firmware_handler *fh)
{
	struct rproc *rproc = container_of(fh, struct rproc, fh);
	struct firmware fw;
	struct device_d *dev;
	int ret;

	if (!rproc) {
		pr_err("invalid rproc handle\n");
		return -EINVAL;
	}

	dev = &rproc->dev;

	dev_info(dev, "powering up %s\n", rproc->name);

	fw.data = rproc->fw_buf;
	fw.size = rproc->fw_buf_ofs;

	ret = rproc_start(rproc, &fw);

	kfree(rproc->fw_buf);

	return ret;
}

int rproc_add(struct rproc *rproc)
{
	struct device_d *dev = &rproc->dev;
	struct firmware_handler *fh;
	int ret;

	fh = &rproc->fh;
	fh->id = xstrdup(rproc->name);
	fh->open = rproc_firmware_start;
	fh->write = rproc_firmware_write_buf;
	fh->close = rproc_firmware_finish;

	ret = firmwaremgr_register(fh);
	if (ret)
		dev_err(dev, "filed to register firmware handler %s\n", rproc->name);
	else
		dev_info(dev, "%s is available\n", rproc->name);

	return ret;
}

struct rproc *rproc_alloc(struct device_d *dev, const char *name,
			  const struct rproc_ops *ops, int len)
{
	struct rproc *rproc;

	if (!dev || !name || !ops)
		return NULL;

	rproc = xzalloc(sizeof(struct rproc) + len);
	if (!rproc) {
		return NULL;
	}

	rproc->ops = kmemdup(ops, sizeof(*ops), GFP_KERNEL);
	if (!rproc->ops) {
		kfree(rproc);
		return NULL;
	}

	rproc->name = name;
	rproc->priv = &rproc[1];

	rproc->dev.parent = dev;
	rproc->dev.priv = rproc;

	/* Assign a unique device index and name */
	rproc->index = 1;

	dev_set_name(&rproc->dev, "remoteproc%d", rproc->index);

	/* Default to ELF loader if no load function is specified */
	if (!rproc->ops->load)
		rproc->ops->load = rproc_elf_load_segments;

	return rproc;
}
