/*
 * nvmem framework provider.
 *
 * Copyright (C) 2015 Srinivas Kandagatla <srinivas.kandagatla@linaro.org>
 * Copyright (C) 2013 Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _LINUX_NVMEM_PROVIDER_H
#define _LINUX_NVMEM_PROVIDER_H

#include <common.h>
#include <linux/types.h>

struct nvmem_device;

struct nvmem_bus {
	int (*write)(struct device_d *dev, const int reg, const void *val,
		     int val_size);
	int (*read)(struct device_d *dev, const int reg, void *val,
		    int val_size);
};

struct nvmem_config {
	struct device_d		*dev;
	const char		*name;
	bool			read_only;
	int			stride;
	int			word_size;
	int			size;
	const struct nvmem_bus	*bus;
};

#if IS_ENABLED(CONFIG_NVMEM)

struct nvmem_device *nvmem_register(const struct nvmem_config *cfg);

#else

static inline struct nvmem_device *nvmem_register(const struct nvmem_config *c)
{
	return ERR_PTR(-ENOSYS);
}

#endif /* CONFIG_NVMEM */
#endif  /* ifndef _LINUX_NVMEM_PROVIDER_H */
