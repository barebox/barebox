// SPDX-License-Identifier: GPL-2.0-only
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/nvmem/meson-efuse.c?id=acc5965b9ff8a1889f5b51466562896d59c6e1b9
/*
 * Amlogic Meson GX eFuse Driver
 *
 * Copyright (c) 2016 Endless Computers, Inc.
 * Author: Carlo Caione <carlo@endlessm.com>
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <of.h>
#include <driver.h>

#include <linux/firmware/meson/meson_sm.h>

static int meson_efuse_read(void *context, unsigned int offset,
			    void *val, size_t bytes)
{
	struct meson_sm_firmware *fw = context;
	int ret;

	ret = meson_sm_call_read(fw, (u8 *)val, bytes, SM_EFUSE_READ, offset,
				 bytes, 0, 0, 0);

	return ret < 0 ? ret : 0;
}

static int meson_efuse_write(void *context, unsigned int offset,
			     const void *val, size_t bytes)
{
	struct meson_sm_firmware *fw = context;
	int ret;

	ret = meson_sm_call_write(fw, (u8 *)val, bytes, SM_EFUSE_WRITE, offset,
				  bytes, 0, 0, 0);

	return ret < 0 ? ret : 0;
}

static const struct of_device_id meson_efuse_match[] = {
	{ .compatible = "amlogic,meson-gxbb-efuse", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, meson_efuse_match);

static int meson_efuse_probe(struct device *dev)
{
	struct meson_sm_firmware *fw;
	struct nvmem_device *nvmem;
	struct nvmem_config *econfig;
	struct clk *clk;
	unsigned int size;
	struct device_node *sm_np __free(device_node) =
			of_parse_phandle(dev->of_node, "secure-monitor", 0);

	if (!sm_np) {
		dev_err(dev, "no secure-monitor node\n");
		return -ENODEV;
	}

	fw = meson_sm_get(sm_np);
	if (!fw)
		return -EPROBE_DEFER;

	clk = clk_get_enabled(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "failed to get efuse gate");

	if (meson_sm_call(fw, SM_EFUSE_USER_MAX, &size, 0, 0, 0, 0, 0) < 0) {
		dev_err(dev, "failed to get max user");
		return -EINVAL;
	}

	econfig = devm_kzalloc(dev, sizeof(*econfig), GFP_KERNEL);
	if (!econfig)
		return -ENOMEM;

	econfig->dev = dev;
	econfig->name = dev_name(dev);
	econfig->stride = 1;
	econfig->word_size = 1;
	econfig->reg_read = meson_efuse_read;
	econfig->reg_write = meson_efuse_write;
	econfig->size = size;
	econfig->priv = fw;

	nvmem = nvmem_register(econfig);

	return PTR_ERR_OR_ZERO(nvmem);
}

static struct driver meson_efuse_driver = {
	.probe = meson_efuse_probe,
	.name = "meson-efuse",
	.of_match_table = meson_efuse_match,
};

device_platform_driver(meson_efuse_driver);

MODULE_AUTHOR("Carlo Caione <carlo@endlessm.com>");
MODULE_DESCRIPTION("Amlogic Meson GX NVMEM driver");
MODULE_LICENSE("GPL v2");
