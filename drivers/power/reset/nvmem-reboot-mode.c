// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Vaisala Oyj. All rights reserved.
 */

#include <common.h>
#include <init.h>
#include <of.h>
#include <linux/nvmem-consumer.h>
#include <linux/reboot-mode.h>

struct nvmem_reboot_mode {
	struct reboot_mode_driver reboot;
	struct nvmem_cell *cell;
};

static int nvmem_reboot_mode_write(struct reboot_mode_driver *reboot,
				   const u32 *_magic)
{
	struct nvmem_reboot_mode *nvmem_rbm;
	u32 magic = *_magic;
	int ret;

	nvmem_rbm = container_of(reboot, struct nvmem_reboot_mode, reboot);

	ret = nvmem_cell_write(nvmem_rbm->cell, &magic, sizeof(magic));
	if (ret < 0)
		dev_err(reboot->dev, "update reboot mode bits failed: %pe\n", ERR_PTR(ret));
	else if (ret != 4)
		ret = -EIO;
	else
		ret = 0;

	return ret;
}

static int nvmem_reboot_mode_probe(struct device *dev)
{
	struct nvmem_reboot_mode *nvmem_rbm;
	struct nvmem_cell *cell;
	void *magicbuf;
	size_t len;
	int ret;

	cell = nvmem_cell_get(dev, "reboot-mode");
	if (IS_ERR(cell))
		return dev_errp_probe(dev, cell, "getting nvmem cell 'reboot-mode'\n");

	nvmem_rbm = xzalloc(sizeof(*nvmem_rbm));

	nvmem_rbm->cell = cell;
	nvmem_rbm->reboot.dev = dev;
	nvmem_rbm->reboot.write = nvmem_reboot_mode_write;
	nvmem_rbm->reboot.priority = 200;

	/*
	 * Fixing up the nvmem reboot device tree node is problematic, because it
	 * contains a phandle to another node. Take the easy way out for now and
	 * expect user to provide matching reboot-mode nodes in both kernel and
	 * barebox device tree manually.
	 */
	nvmem_rbm->reboot.no_fixup = true;

	magicbuf = nvmem_cell_read(nvmem_rbm->cell, &len);
	if (IS_ERR(magicbuf) || len != 4) {
		dev_err(dev, "error reading reboot mode: %pe\n", magicbuf);
		return PTR_ERR(magicbuf);
	}

	ret = reboot_mode_register(&nvmem_rbm->reboot, magicbuf, 1);
	if (ret)
		dev_err(dev, "can't register reboot mode\n");

	free(magicbuf);

	return ret;
}

static const struct of_device_id nvmem_reboot_mode_of_match[] = {
	{ .compatible = "nvmem-reboot-mode" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, nvmem_reboot_mode_of_match);

static struct driver nvmem_reboot_mode_driver = {
	.probe = nvmem_reboot_mode_probe,
	.name = "nvmem-reboot-mode",
	.of_compatible = nvmem_reboot_mode_of_match,
};
coredevice_platform_driver(nvmem_reboot_mode_driver);
