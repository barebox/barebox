// SPDX-License-Identifier: GPL-2.0
/*
 * RTC subsystem, nvmem interface
 *
 * Copyright (C) 2017 Alexandre Belloni
 */

#include <linux/err.h>
#include <linux/types.h>
#include <linux/nvmem-provider.h>
#include <linux/rtc.h>

int rtc_nvmem_register(struct rtc_device *rtc,
		       struct nvmem_config *nvmem_config)
{
	struct device *dev = rtc->dev;
	struct nvmem_device *nvmem;

	if (!nvmem_config)
		return -ENODEV;

	nvmem_config->dev = dev;
	nvmem = nvmem_register(nvmem_config);
	if (IS_ERR(nvmem))
		dev_err(dev, "failed to register nvmem device for RTC\n");

	return PTR_ERR_OR_ZERO(nvmem);
}
EXPORT_SYMBOL_GPL(rtc_nvmem_register);
