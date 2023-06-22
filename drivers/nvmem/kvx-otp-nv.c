// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Kalray Inc., Clément Léger
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <net.h>
#include <io.h>

#include <linux/nvmem-provider.h>

#define OTP_NV_ALIGN		4
#define OTP_NV_ALIGN_MASK	(OTP_NV_ALIGN - 1)

struct kvx_otp_nv_priv {
	void __iomem *base;
};

static int kvx_otp_nv_read(void *context, unsigned int offset,
			  void *_val, size_t bytes)
{
	struct kvx_otp_nv_priv *priv = context;
	u8 *val = _val;
	u32 tmp, copy_size;
	u8 skip = offset & OTP_NV_ALIGN_MASK;

	offset &= ~OTP_NV_ALIGN_MASK;

	while (bytes) {
		tmp = readl(priv->base + offset);
		if (skip != 0)
			copy_size = min(OTP_NV_ALIGN - skip, (int) bytes);
		else
			copy_size = min(bytes, sizeof(tmp));

		memcpy(val, ((u8 *) &tmp) + skip, copy_size);
		skip = 0;

		bytes -= copy_size;
		val += copy_size;
		offset += OTP_NV_ALIGN;
	}

	return 0;
}

static const struct of_device_id kvx_otp_nv_match[] = {
	{ .compatible = "kalray,kvx-otp-nv" },
	{ /* sentinel */},
};
MODULE_DEVICE_TABLE(of, kvx_otp_nv_match);

static int kvx_otp_nv_probe(struct device *dev)
{
	struct resource *res;
	struct nvmem_device *nvmem;
	struct nvmem_config econfig = { 0 };
	struct kvx_otp_nv_priv *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	priv->base = IOMEM(res->start);

	econfig.name = "kvx-nv-regbank";
	econfig.stride = 1;
	econfig.word_size = 1;
	econfig.size = resource_size(res);
	econfig.dev = dev;
	econfig.priv = priv;
	econfig.reg_read = kvx_otp_nv_read;

	dev->priv = priv;

	nvmem = nvmem_register(&econfig);

	return PTR_ERR_OR_ZERO(nvmem);
}

static struct driver kvx_otp_nv_driver = {
	.name	= "kvx-otp-nv",
	.probe	= kvx_otp_nv_probe,
	.of_compatible = DRV_OF_COMPAT(kvx_otp_nv_match),
};
postcore_platform_driver(kvx_otp_nv_driver);
