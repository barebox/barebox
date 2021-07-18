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
	int (*write)(void *ctx, unsigned int reg, const void *val, size_t val_size);
	int (*read)(void *ctx, unsigned int reg, void *val, size_t val_size);
};

struct nvmem_config {
	struct device_d		*dev;
	const char		*name;
	bool			read_only;
	struct cdev		*cdev;
	int			stride;
	int			word_size;
	int			size;
	const struct nvmem_bus	*bus;
	void			*priv;
};

struct regmap;
struct cdev;

#if IS_ENABLED(CONFIG_NVMEM)

struct nvmem_device *nvmem_register(const struct nvmem_config *cfg);
struct nvmem_device *nvmem_regmap_register(struct regmap *regmap, const char *name);
struct nvmem_device *nvmem_partition_register(struct cdev *cdev);

#else

static inline struct nvmem_device *nvmem_register(const struct nvmem_config *c)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct nvmem_device *nvmem_regmap_register(struct regmap *regmap, const char *name)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct nvmem_device *nvmem_partition_register(struct cdev *cdev)
{
	return ERR_PTR(-ENOSYS);
}

#endif /* CONFIG_NVMEM */
#endif  /* ifndef _LINUX_NVMEM_PROVIDER_H */
