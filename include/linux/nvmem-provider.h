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
#include <linux/device.h>

struct nvmem_device;
typedef int (*nvmem_reg_read_t)(void *priv, unsigned int offset,
				void *val, size_t bytes);
typedef int (*nvmem_reg_write_t)(void *priv, unsigned int offset,
				 const void *val, size_t bytes);
/* used for vendor specific post processing of cell data */
typedef int (*nvmem_cell_post_process_t)(void *priv, const char *id, int index,
					 unsigned int offset, void *buf,
					 size_t bytes);

#define NVMEM_DEVID_NONE	(-1)
#define NVMEM_DEVID_AUTO	(-2)

/**
 * struct nvmem_cell_info - NVMEM cell description
 * @name:	Name.
 * @offset:	Offset within the NVMEM device.
 * @raw_len:	Length of raw data (without post processing).
 * @bytes:	Length of the cell.
 * @bit_offset:	Bit offset if cell is smaller than a byte.
 * @nbits:	Number of bits.
 * @np:		Optional device_node pointer.
 * @read_post_process:	Callback for optional post processing of cell data
 *			on reads.
 * @priv:	Opaque data passed to the read_post_process hook.
 */
struct nvmem_cell_info {
	const char		*name;
	unsigned int		offset;
	size_t			raw_len;
	unsigned int		bytes;
	unsigned int		bit_offset;
	unsigned int		nbits;
	struct device_node	*np;
	nvmem_cell_post_process_t read_post_process;
	void			*priv;
};

/**
 * struct nvmem_config - NVMEM device configuration
 *
 * @dev:	Parent device.
 * @name:	Optional name.
 * @id:		Optional device ID used in full name. Ignored if name is NULL.
 * @fixup_dt_cell_info: Will be called before a cell is added. Can be
 *		used to modify the nvmem_cell_info.
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
	int			id;
	void (*fixup_dt_cell_info)(struct nvmem_device *nvmem,
				   struct nvmem_cell_info *cell);
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
};

/**
 * struct nvmem_layout - NVMEM layout definitions
 *
 * @dev:		Device-model layout device.
 * @nvmem:		The underlying NVMEM device
 * @add_cells:		Will be called if a nvmem device is found which
 *			has this layout. The function will add layout
 *			specific cells with nvmem_add_one_cell().
 *
 * A nvmem device can hold a well defined structure which can just be
 * evaluated during runtime. For example a TLV list, or a list of "name=val"
 * pairs. A nvmem layout can parse the nvmem device and add appropriate
 * cells.
 */
struct nvmem_layout {
	struct device dev;
	struct nvmem_device *nvmem;
	int (*add_cells)(struct nvmem_layout *layout);
};

struct nvmem_layout_driver {
	struct device_driver driver;
	int (*probe)(struct nvmem_layout *layout);
	void (*remove)(struct nvmem_layout *layout);
};

struct regmap;
struct cdev;

#if IS_ENABLED(CONFIG_NVMEM)

struct nvmem_device *nvmem_register(const struct nvmem_config *cfg);
struct nvmem_device *nvmem_regmap_register(struct regmap *regmap, const char *name);
struct nvmem_device *nvmem_regmap_register_with_pp(struct regmap *map, const char *name,
			      void (*fixup_dt_cell_info)(struct nvmem_device *nvmem,
							 struct nvmem_cell_info *cell));
struct device *nvmem_device_get_device(struct nvmem_device *nvmem);
int nvmem_add_one_cell(struct nvmem_device *nvmem,
		       const struct nvmem_cell_info *info);

int nvmem_layout_register(struct nvmem_layout *layout);
void nvmem_layout_unregister(struct nvmem_layout *layout);

int nvmem_layout_driver_register(struct nvmem_layout_driver *drv);
#define module_nvmem_layout_driver(drv)		\
	register_driver_macro(device, nvmem_layout, drv)

#else

static inline struct nvmem_device *nvmem_register(const struct nvmem_config *c)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct nvmem_device *nvmem_regmap_register(struct regmap *regmap, const char *name)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct nvmem_device *nvmem_regmap_register_with_pp(struct regmap *map,
		const char *name, void (*fixup_dt_cell_info)(struct nvmem_device *nvmem,
							     struct nvmem_cell_info *cell))
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

static inline int nvmem_layout_register(struct nvmem_layout *layout)
{
	return -EOPNOTSUPP;
}

static inline void nvmem_layout_unregister(struct nvmem_layout *layout) {}

#endif /* CONFIG_NVMEM */

#if IS_ENABLED(CONFIG_NVMEM) && IS_ENABLED(CONFIG_OF)

/**
 * of_nvmem_layout_get_container() - Get OF node of layout container
 *
 * @nvmem: nvmem device
 *
 * Return: a node pointer with refcount incremented or NULL if no
 * container exists. Use of_node_put() on it when done.
 */
struct device_node *of_nvmem_layout_get_container(struct nvmem_device *nvmem);

#else  /* CONFIG_NVMEM && CONFIG_OF */

static inline struct device_node *of_nvmem_layout_get_container(struct nvmem_device *nvmem)
{
	return NULL;
}

#endif /* CONFIG_NVMEM && CONFIG_OF */

#endif  /* ifndef _LINUX_NVMEM_PROVIDER_H */
