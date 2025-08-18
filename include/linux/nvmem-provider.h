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
typedef int (*nvmem_reg_read_t)(void *priv, unsigned int offset,
				void *val, size_t bytes);
typedef int (*nvmem_reg_write_t)(void *priv, unsigned int offset,
				 const void *val, size_t bytes);
/* used for vendor specific post processing of cell data */
typedef int (*nvmem_cell_post_process_t)(void *priv, const char *id,
					 unsigned int offset, void *buf,
					 size_t bytes);

/**
 * struct nvmem_cell_info - NVMEM cell description
 * @name:	Name.
 * @offset:	Offset within the NVMEM device.
 * @raw_len:	Length of raw data (without post processing).
 * @bytes:	Length of the cell.
 * @bit_offset:	Bit offset if cell is smaller than a byte.
 * @nbits:	Number of bits.
 * @np:		Optional device_node pointer.
 */
struct nvmem_cell_info {
	const char		*name;
	unsigned int		offset;
	size_t			raw_len;
	unsigned int		bytes;
	unsigned int		bit_offset;
	unsigned int		nbits;
	struct device_node	*np;
};

/**
 * struct nvmem_config - NVMEM device configuration
 *
 * @dev:	Parent device.
 * @name:	Optional name.
 * @id:		Optional device ID used in full name. Ignored if name is NULL.
 * @read_only:	Device is read-only.
 * @reg_read:	Callback to read data; return zero if successful.
 * @reg_write:	Callback to write data; return zero if successful.
 * @size:	Device size.
 * @word_size:	Minimum read/write access granularity.
 * @stride:	Minimum read/write access stride.
 * @priv:	User context passed to read/write callbacks.
 */
struct nvmem_config {
	struct device		*dev;
	const char		*name;
	bool			read_only;
	struct cdev		*cdev;
	nvmem_reg_read_t	reg_read;
	nvmem_reg_write_t	reg_write;
	int			size;
	int			(*reg_protect)(void *ctx, unsigned int offset,
					       size_t bytes, int prot);
	int			word_size;
	int			stride;
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
struct device *nvmem_device_get_device(struct nvmem_device *nvmem);
int nvmem_add_one_cell(struct nvmem_device *nvmem,
		       const struct nvmem_cell_info *info);

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

static inline struct device *nvmem_device_get_device(struct nvmem_device *nvmem)
{
	return ERR_PTR(-ENOSYS);
}

static inline int nvmem_add_one_cell(struct nvmem_device *nvmem,
				     const struct nvmem_cell_info *info)
{
	return -EOPNOTSUPP;
}

#endif /* CONFIG_NVMEM */
#endif  /* ifndef _LINUX_NVMEM_PROVIDER_H */
