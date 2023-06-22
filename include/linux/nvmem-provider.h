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

/* used for vendor specific post processing of cell data */
typedef int (*nvmem_cell_post_process_t)(void *priv, const char *id,
					 unsigned int offset, void *buf,
					 size_t bytes);

struct nvmem_config {
	struct device		*dev;
	const char		*name;
	bool			read_only;
	struct cdev		*cdev;
	int			stride;
	int			word_size;
	int			size;
	int			(*reg_write)(void *ctx, unsigned int reg,
					     const void *val, size_t val_size);
	int			(*reg_read)(void *ctx, unsigned int reg,
					    void *val, size_t val_size);
	void			*priv;
	nvmem_cell_post_process_t cell_post_process;
};

struct regmap;
struct cdev;

#if IS_ENABLED(CONFIG_NVMEM)

struct nvmem_device *nvmem_register(const struct nvmem_config *cfg);
struct nvmem_device *nvmem_regmap_register(struct regmap *regmap, const char *name);
struct nvmem_device *nvmem_regmap_register_with_pp(struct regmap *regmap,
		const char *name, nvmem_cell_post_process_t cell_post_process);
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

static inline struct nvmem_device *
nvmem_regmap_register_with_pp(struct regmap *regmap, const char *name,
			      nvmem_cell_post_process_t cell_post_process)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct nvmem_device *nvmem_partition_register(struct cdev *cdev)
{
	return ERR_PTR(-ENOSYS);
}

#endif /* CONFIG_NVMEM */
#endif  /* ifndef _LINUX_NVMEM_PROVIDER_H */
