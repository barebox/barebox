// SPDX-License-Identifier: GPL-2.0-only
/*
 * nvmem framework core.
 *
 * Copyright (C) 2015 Srinivas Kandagatla <srinivas.kandagatla@linaro.org>
 * Copyright (C) 2013 Maxime Ripard <maxime.ripard@free-electrons.com>
 */

#include <common.h>
#include <libbb.h>
#include <malloc.h>
#include <of.h>
#include <linux/nvmem-consumer.h>
#include <linux/nvmem-provider.h>

#include "internals.h"

struct nvmem_cell_entry {
	const char		*name;
	int			offset;
	size_t			raw_len;
	int			bytes;
	int			bit_offset;
	int			nbits;
	nvmem_cell_post_process_t read_post_process;
	void			*priv;
	struct device_node	*np;
	struct nvmem_device	*nvmem;
	struct list_head	node;

	struct cdev		cdev;
};

struct nvmem_cell {
	struct nvmem_cell_entry *entry;
	const char		*id;
	int			index;
};

DEFINE_DEV_CLASS(nvmem_class, "nvmem");

void nvmem_devices_print(void)
{
	struct nvmem_device *dev;

	class_for_each_container_of_device(&nvmem_class, dev, dev)
		printf("%s\n", dev_name(&dev->dev));
}

static ssize_t nvmem_cdev_read(struct cdev *cdev, void *buf, size_t count,
			       loff_t offset, unsigned long flags)
{
	struct nvmem_device *nvmem;
	ssize_t retlen;

	nvmem = container_of(cdev, struct nvmem_device, cdev);

	dev_dbg(cdev->dev, "read ofs: 0x%08llx count: 0x%08zx\n",
		offset, count);

	retlen = nvmem_device_read(nvmem, offset, count, buf);

	return retlen;
}

static ssize_t nvmem_cdev_write(struct cdev *cdev, const void *buf, size_t count,
				loff_t offset, unsigned long flags)
{
	struct nvmem_device *nvmem;
	ssize_t retlen;

	nvmem = container_of(cdev, struct nvmem_device, cdev);

	dev_dbg(cdev->dev, "write ofs: 0x%08llx count: 0x%08zx\n",
		offset, count);

	retlen = nvmem_device_write(nvmem, offset, count, buf);

	return retlen;
}

static int nvmem_cdev_protect(struct cdev *cdev, size_t count, loff_t offset,
			      int prot)
{
	struct nvmem_device *nvmem;

	nvmem = container_of(cdev, struct nvmem_device, cdev);

	dev_dbg(cdev->dev, "protect ofs: 0x%08llx count: 0x%08zx prot: %d\n",
		offset, count, prot);

	if (!nvmem->reg_protect) {
		dev_warn(cdev->dev, "NVMEM device %s does not support protect operation\n",
			 nvmem->name);
		return -EOPNOTSUPP;
	}

	if (!count)
		return 0;

	if (offset + count > nvmem->size) {
		dev_err(cdev->dev, "protect range out of bounds (ofs: 0x%08llx, count 0x%08zx, size 0x%08zx)\n",
			offset, count, nvmem->size);
		return -EINVAL;
	}

	return nvmem->reg_protect(nvmem->priv, offset, count, prot);
}

static struct cdev_operations nvmem_chrdev_ops = {
	.read  = nvmem_cdev_read,
	.write  = nvmem_cdev_write,
	.protect = nvmem_cdev_protect,
};

static int nvmem_register_cdev(struct nvmem_device *nvmem, const char *name)
{
	struct device *dev = &nvmem->dev;
	struct cdev *cdev = &nvmem->cdev;
	const char *alias;
	int ret;

	alias = of_alias_get(dev->of_node);

	cdev->name = xstrdup(alias ?: name);
	cdev->ops = &nvmem_chrdev_ops;
	cdev->dev = dev;
	cdev->size = nvmem->size;

	ret = devfs_create(cdev);
	if (ret)
		return ret;

	of_parse_partitions(cdev, dev->of_node);
	of_partitions_register_fixup(cdev);

	return 0;
}

static struct nvmem_device *of_nvmem_find(struct device_node *nvmem_np)
{
	struct nvmem_device *dev;

	if (!nvmem_np)
		return NULL;

	class_for_each_container_of_device(&nvmem_class, dev, dev)
		if (dev->dev.of_node == nvmem_np)
			return dev;

	return NULL;
}

static struct nvmem_cell *nvmem_create_cell(struct nvmem_cell_entry *entry,
					    const char *id, int index);

static ssize_t nvmem_cell_cdev_read(struct cdev *cdev, void *buf, size_t count,
				    loff_t offset, unsigned long flags)
{
	struct nvmem_cell_entry *entry;
	struct nvmem_cell *cell = NULL;
	size_t cell_sz, read_len;
	void *content;

	entry = container_of(cdev, struct nvmem_cell_entry, cdev);
	cell = nvmem_create_cell(entry, entry->name, 0);
	if (IS_ERR(cell))
		return PTR_ERR(cell);

	content = nvmem_cell_read(cell, &cell_sz);
	if (IS_ERR(content)) {
		read_len = PTR_ERR(content);
		goto destroy_cell;
	}

	read_len = min_t(unsigned int, cell_sz - offset, count);
	memcpy(buf, content + offset, read_len);
	kfree(content);

destroy_cell:
	kfree_const(cell->id);
	kfree(cell);

	return read_len;
}

static struct cdev_operations nvmem_cell_chrdev_ops = {
	.read  = nvmem_cell_cdev_read,
};

static int nvmem_populate_sysfs_cells(struct nvmem_device *nvmem)
{
	struct device *dev = &nvmem->dev;
	struct nvmem_cell_entry *entry;

	if (list_empty(&nvmem->cells) || nvmem->sysfs_cells_populated)
		return 0;

	list_for_each_entry(entry, &nvmem->cells, node) {
		struct cdev *cdev;
		int ret;

		cdev = &entry->cdev;
		cdev->name = xasprintf("%s.%s", dev_name(dev),
				       kbasename(entry->name));
		cdev->ops = &nvmem_cell_chrdev_ops;
		cdev->dev = dev;
		cdev->size = entry->bytes;

		/* Ignore the error but throw a warning for the user */
		ret = devfs_create(cdev);
		if (ret) {
			dev_err(dev, "Failed to register %s with %pe\n",
				cdev->name, ERR_PTR(ret));
			return ret;
		}
	}

	nvmem->sysfs_cells_populated = true;

	return 0;
}

static void nvmem_cell_entry_drop(struct nvmem_cell_entry *cell)
{
	list_del(&cell->node);
	of_node_put(cell->np);
	kfree_const(cell->name);
	kfree(cell);
}

static void nvmem_device_remove_all_cells(const struct nvmem_device *nvmem)
{
	struct nvmem_cell_entry *cell, *p;

	list_for_each_entry_safe(cell, p, &nvmem->cells, node)
		nvmem_cell_entry_drop(cell);
}

static void nvmem_device_remove_all_cdevs(struct nvmem_device *nvmem)
{
	struct nvmem_cell_entry *cell, *p;

	list_for_each_entry_safe(cell, p, &nvmem->cells, node) {
		if (cell->cdev.name && cdev_by_name(cell->cdev.name)) {
			devfs_remove(&cell->cdev);
			free(cell->cdev.name);
			cell->cdev.name = NULL;
		}
	}

	if (!nvmem->cdev.name)
		return;

	devfs_remove(&nvmem->cdev);
	free(nvmem->cdev.name);
	nvmem->cdev.name = NULL;
}

static void nvmem_cell_entry_add(struct nvmem_cell_entry *cell)
{
	list_add_tail(&cell->node, &cell->nvmem->cells);
}

static int nvmem_cell_info_to_nvmem_cell_entry_nodup(struct nvmem_device *nvmem,
						     const struct nvmem_cell_info *info,
						     struct nvmem_cell_entry *cell)
{
	cell->nvmem = nvmem;
	cell->offset = info->offset;
	cell->raw_len = info->raw_len ?: info->bytes;
	cell->bytes = info->bytes;
	cell->name = info->name;
	cell->read_post_process = info->read_post_process;
	cell->priv = info->priv;

	cell->bit_offset = info->bit_offset;
	cell->nbits = info->nbits;
	cell->np = info->np;

	if (cell->nbits)
		cell->bytes = DIV_ROUND_UP(cell->nbits + cell->bit_offset,
					   BITS_PER_BYTE);

	if (!IS_ALIGNED(cell->offset, nvmem->stride)) {
		dev_err(&nvmem->dev,
			"cell %s unaligned to nvmem stride %d\n",
			cell->name ?: "<unknown>", nvmem->stride);
		return -EINVAL;
	}

	return 0;
}

static int nvmem_cell_info_to_nvmem_cell_entry(struct nvmem_device *nvmem,
					       const struct nvmem_cell_info *info,
					       struct nvmem_cell_entry *cell)
{
	int err;

	err = nvmem_cell_info_to_nvmem_cell_entry_nodup(nvmem, info, cell);
	if (err)
		return err;

	cell->name = kstrdup_const(info->name, GFP_KERNEL);
	if (!cell->name)
		return -ENOMEM;

	return 0;
}

/**
 * nvmem_add_one_cell() - Add one cell information to an nvmem device
 *
 * @nvmem: nvmem device to add cells to.
 * @info: nvmem cell info to add to the device
 *
 * Return: 0 or negative error code on failure.
 */
int nvmem_add_one_cell(struct nvmem_device *nvmem,
		       const struct nvmem_cell_info *info)
{
	struct nvmem_cell_entry *cell;
	int rval;

	cell = kzalloc(sizeof(*cell), GFP_KERNEL);
	if (!cell)
		return -ENOMEM;

	rval = nvmem_cell_info_to_nvmem_cell_entry(nvmem, info, cell);
	if (rval) {
		kfree(cell);
		return rval;
	}

	nvmem_cell_entry_add(cell);

	return 0;
}
EXPORT_SYMBOL_GPL(nvmem_add_one_cell);

static int nvmem_add_cells_from_dt(struct nvmem_device *nvmem, struct device_node *np)
{
	struct device *dev = &nvmem->dev;
	struct device_node *child;
	const __be32 *addr;
	int len, ret;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return 0;

	if (!np)
		return 0;

	for_each_child_of_node(np, child) {
		struct nvmem_cell_info info = {0};

		addr = of_get_property(child, "reg", &len);
		if (!addr)
			continue;
		if (len < 2 * sizeof(u32)) {
			dev_err(dev, "nvmem: invalid reg on %pOF\n", child);
			of_node_put(child);
			return -EINVAL;
		}

		info.offset = be32_to_cpup(addr++);
		info.bytes = be32_to_cpup(addr);
		info.name = basprintf("%pOFn", child);

		addr = of_get_property(child, "bits", &len);
		if (addr && len == (2 * sizeof(u32))) {
			info.bit_offset = be32_to_cpup(addr++);
			info.nbits = be32_to_cpup(addr);
		}

		info.np = of_node_get(child);

		if (nvmem->fixup_dt_cell_info)
			nvmem->fixup_dt_cell_info(nvmem, &info);

		ret = nvmem_add_one_cell(nvmem, &info);
		kfree(info.name);
		if (ret) {
			of_node_put(child);
			return ret;
		}
	}

	return 0;
}

static int nvmem_add_cells_from_legacy_of(struct nvmem_device *nvmem)
{
	return nvmem_add_cells_from_dt(nvmem, nvmem->dev.of_node);
}

static int nvmem_add_cells_from_fixed_layout(struct nvmem_device *nvmem)
{
	struct device_node *layout_np;
	int err = 0;

	layout_np = of_nvmem_layout_get_container(nvmem);
	if (!layout_np)
		return 0;

	if (of_device_is_compatible(layout_np, "fixed-layout"))
		err = nvmem_add_cells_from_dt(nvmem, layout_np);

	of_node_put(layout_np);

	return err;
}

int nvmem_layout_register(struct nvmem_layout *layout)
{
	int ret;

	if (!layout->add_cells)
		return -EINVAL;

	/* Populate the cells */
	ret = layout->add_cells(layout);
	if (ret)
		return ret;

	ret = nvmem_populate_sysfs_cells(layout->nvmem);
	if (ret) {
		nvmem_device_remove_all_cdevs(layout->nvmem);
		nvmem_device_remove_all_cells(layout->nvmem);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(nvmem_layout_register);

void nvmem_layout_unregister(struct nvmem_layout *layout)
{
	/* Keep the API even with an empty stub in case we need it later */
}
EXPORT_SYMBOL_GPL(nvmem_layout_unregister);

/**
 * nvmem_register() - Register a nvmem device for given nvmem_config.
 *
 * @config: nvmem device configuration with which nvmem device is created.
 *
 * Return: Will be an ERR_PTR() on error or a valid pointer to nvmem_device
 * on success.
 */

struct nvmem_device *nvmem_register(const struct nvmem_config *config)
{
	struct nvmem_device *nvmem;
	struct device_node *np;
	int rval;

	if (!config->dev)
		return ERR_PTR(-EINVAL);

	nvmem = kzalloc(sizeof(*nvmem), GFP_KERNEL);
	if (!nvmem)
		return ERR_PTR(-ENOMEM);

	nvmem->stride = config->stride;
	nvmem->word_size = config->word_size;
	nvmem->size = config->size;
	nvmem->dev.parent = config->dev;
	nvmem->reg_read = config->reg_read;
	nvmem->reg_write = config->reg_write;
	nvmem->reg_protect = config->reg_protect;
	np = config->cdev ? cdev_of_node(config->cdev) : config->dev->of_node;
	nvmem->dev.of_node = np;
	nvmem->priv = config->priv;
	INIT_LIST_HEAD(&nvmem->cells);
	nvmem->fixup_dt_cell_info = config->fixup_dt_cell_info;

	rval = nvmem_add_cells_from_legacy_of(nvmem);
	if (rval)
		goto err_remove_cells;

	rval = nvmem_add_cells_from_fixed_layout(nvmem);
	if (rval)
		goto err_remove_cells;

	if (config->read_only || !config->reg_write || of_property_read_bool(np, "read-only"))
		nvmem->read_only = true;

	if (config->name) {
		dev_set_name(&nvmem->dev, "%s", config->name);

		switch (config->id) {
		case NVMEM_DEVID_NONE:
			nvmem->dev.id = DEVICE_ID_SINGLE;
			break;
		case NVMEM_DEVID_AUTO:
			nvmem->dev.id = DEVICE_ID_DYNAMIC;
			break;
		default:
			nvmem->dev.id = config->id;
		break;
		}
	} else {
		dev_set_name(&nvmem->dev, "nvmem");
		nvmem->dev.id = DEVICE_ID_DYNAMIC;
	}

	rval = register_device(&nvmem->dev);
	if (rval)
		goto err_remove_cells;

	if (!config->cdev) {
		rval = nvmem_register_cdev(nvmem, dev_name(&nvmem->dev));
		if (rval)
			goto err_unregister;
	}

	rval = nvmem_populate_layout(nvmem);
	if (rval)
		goto err_remove_cdevs;

	rval = nvmem_populate_sysfs_cells(nvmem);
	if (rval)
		goto err_destroy_layout;

	class_add_device(&nvmem_class, &nvmem->dev);

	return nvmem;

err_destroy_layout:
	nvmem_destroy_layout(nvmem);
err_remove_cdevs:
	nvmem_device_remove_all_cdevs(nvmem);
err_unregister:
	unregister_device(&nvmem->dev);
err_remove_cells:
	nvmem_device_remove_all_cells(nvmem);
	kfree(nvmem);

	return ERR_PTR(rval);
}
EXPORT_SYMBOL_GPL(nvmem_register);

static int of_nvmem_device_ensure_probed(struct device_node *np)
{
	if (of_device_is_compatible(np, "nvmem-cells"))
		return of_partition_ensure_probed(np);

	return of_device_ensure_probed(np);
}

static struct nvmem_device *__nvmem_device_get(struct device_node *np)
{
	struct nvmem_device *nvmem = NULL;
	int ret;

	if (!np)
		return ERR_PTR(-EINVAL);

	ret = of_nvmem_device_ensure_probed(np);
	if (ret)
		return ERR_PTR(ret);

	nvmem = of_nvmem_find(np);
	if (!nvmem)
		return ERR_PTR(-EPROBE_DEFER);

	nvmem->users++;

	return nvmem;
}

static void __nvmem_device_put(struct nvmem_device *nvmem)
{
	nvmem->users--;
}

static struct nvmem_device *nvmem_find(const char *name)
{
	return ERR_PTR(-ENOSYS);
}

#if IS_ENABLED(CONFIG_NVMEM) && IS_ENABLED(CONFIG_OFTREE)
/**
 * of_nvmem_device_get() - Get nvmem device from a given id
 *
 * @dev node: Device tree node that uses the nvmem device
 * @id: nvmem name from nvmem-names property.
 *
 * Return: ERR_PTR() on error or a valid pointer to a struct nvmem_device
 * on success.
 */
struct nvmem_device *of_nvmem_device_get(struct device_node *np, const char *id)
{

	struct device_node *nvmem_np;
	struct nvmem_device *nvmem;
	int index = 0;

	if (id)
		index = of_property_match_string(np, "nvmem-names", id);

	nvmem_np = of_parse_phandle(np, "nvmem", index);
	if (!nvmem_np)
		return ERR_PTR(-ENOENT);

	nvmem = __nvmem_device_get(nvmem_np);
	of_node_put(nvmem_np);
	return nvmem;
}
EXPORT_SYMBOL_GPL(of_nvmem_device_get);
#endif

/**
 * nvmem_device_get() - Get nvmem device from a given id
 *
 * @dev : Device that uses the nvmem device
 * @id: nvmem name from nvmem-names property.
 *
 * Return: ERR_PTR() on error or a valid pointer to a struct nvmem_device
 * on success.
 */
struct nvmem_device *nvmem_device_get(struct device *dev,
				      const char *dev_name)
{
	if (dev->of_node) { /* try dt first */
		struct nvmem_device *nvmem;

		nvmem = of_nvmem_device_get(dev->of_node, dev_name);

		if (!IS_ERR(nvmem) || PTR_ERR(nvmem) == -EPROBE_DEFER)
			return nvmem;

	}

	return nvmem_find(dev_name);
}
EXPORT_SYMBOL_GPL(nvmem_device_get);

struct nvmem_device *nvmem_from_device(struct device *dev)
{
	return container_of_safe(dev, struct nvmem_device, dev);
}
EXPORT_SYMBOL_GPL(nvmem_from_device);

/**
 * nvmem_device_put() - put alredy got nvmem device
 *
 * @nvmem: pointer to nvmem device that needs to be released.
 */
void nvmem_device_put(struct nvmem_device *nvmem)
{
	__nvmem_device_put(nvmem);
}
EXPORT_SYMBOL_GPL(nvmem_device_put);

static struct nvmem_cell *nvmem_create_cell(struct nvmem_cell_entry *entry,
					    const char *id, int index)
{
	struct nvmem_cell *cell;
	const char *name = NULL;

	cell = kzalloc(sizeof(*cell), GFP_KERNEL);
	if (!cell)
		return ERR_PTR(-ENOMEM);

	if (id) {
		name = kstrdup_const(id, GFP_KERNEL);
		if (!name) {
			kfree(cell);
			return ERR_PTR(-ENOMEM);
		}
	}

	cell->id = name;
	cell->entry = entry;
	cell->index = index;

	return cell;
}

#if IS_ENABLED(CONFIG_NVMEM) && IS_ENABLED(CONFIG_OFTREE)
static struct nvmem_cell_entry *
nvmem_find_cell_entry_by_node(struct nvmem_device *nvmem, struct device_node *np)
{
	struct nvmem_cell_entry *iter, *cell = NULL;

	list_for_each_entry(iter, &nvmem->cells, node) {
		if (np == iter->np) {
			cell = iter;
			break;
		}
	}

	return cell;
}

static int nvmem_layout_module_get_optional(struct nvmem_device *nvmem)
{
	if (!nvmem->layout)
		return 0;

	if (!nvmem->layout->dev.driver)
		return -EPROBE_DEFER;

	return 0;
}

/**
 * of_nvmem_cell_get() - Get a nvmem cell from given device node and cell id
 *
 * @np: Device tree node that uses the nvmem cell.
 * @id: nvmem cell name from nvmem-cell-names property, or NULL
 *      for the cell at index 0 (the lone cell with no accompanying
 *      nvmem-cell-names property).
 *
 * Return: Will be an ERR_PTR() on error or a valid pointer
 * to a struct nvmem_cell.  The nvmem_cell will be freed by the
 * nvmem_cell_put().
 */
struct nvmem_cell *of_nvmem_cell_get(struct device_node *np, const char *id)
{
	struct device_node *cell_np, *nvmem_np;
	struct nvmem_device *nvmem;
	struct nvmem_cell_entry *cell_entry;
	struct nvmem_cell *cell;
	struct of_phandle_args cell_spec;
	int ret, index = 0;
	int cell_index = 0;

	/* if cell name exists, find index to the name */
	if (id)
		index = of_property_match_string(np, "nvmem-cell-names", id);

	ret = of_parse_phandle_with_optional_args(np, "nvmem-cells",
						  "#nvmem-cell-cells",
						  index, &cell_spec);
	if (ret)
		return ERR_PTR(ret);

	if (cell_spec.args_count > 1)
		return ERR_PTR(-EINVAL);

	cell_np = cell_spec.np;
	if (cell_spec.args_count)
		cell_index = cell_spec.args[0];

	nvmem_np = of_get_parent(cell_np);
	if (!nvmem_np) {
		of_node_put(cell_np);
		return ERR_PTR(-EINVAL);
	}

	/* nvmem layouts produce cells within the nvmem-layout container */
	if (of_node_name_eq(nvmem_np, "nvmem-layout")) {
		nvmem_np = of_get_parent(nvmem_np);
		if (!nvmem_np) {
			of_node_put(cell_np);
			return ERR_PTR(-EINVAL);
		}
	}

	nvmem = __nvmem_device_get(nvmem_np);
	of_node_put(nvmem_np);
	if (IS_ERR(nvmem)) {
		of_node_put(cell_np);
		return ERR_CAST(nvmem);
	}

	ret = nvmem_layout_module_get_optional(nvmem);
	if (ret) {
		of_node_put(cell_np);
		__nvmem_device_put(nvmem);
		return ERR_PTR(ret);
	}

	cell_entry = nvmem_find_cell_entry_by_node(nvmem, cell_np);
	of_node_put(cell_np);
	if (!cell_entry) {
		__nvmem_device_put(nvmem);
		return ERR_PTR(-ENOENT);
	}

	cell = nvmem_create_cell(cell_entry, id, cell_index);
	if (IS_ERR(cell))
		__nvmem_device_put(nvmem);

	return cell;
}
EXPORT_SYMBOL_GPL(of_nvmem_cell_get);
#endif

/**
 * nvmem_cell_get() - Get nvmem cell of device form a given cell name
 *
 * @dev: Device that requests the nvmem cell.
 * @id: nvmem cell name to get (this corresponds with the name from the
 *      nvmem-cell-names property for DT systems)
 *
 * Return: Will be an ERR_PTR() on error or a valid pointer
 * to a struct nvmem_cell.  The nvmem_cell will be freed by the
 * nvmem_cell_put().
 */
struct nvmem_cell *nvmem_cell_get(struct device *dev, const char *cell_id)
{
	struct nvmem_cell *cell;

	if (dev->of_node) { /* try dt first */
		cell = of_nvmem_cell_get(dev->of_node, cell_id);
		if (!IS_ERR(cell) || PTR_ERR(cell) == -EPROBE_DEFER)
			return cell;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(nvmem_cell_get);

/**
 * nvmem_cell_put() - Release previously allocated nvmem cell.
 *
 * @cell: Previously allocated nvmem cell by nvmem_cell_get().
 */
void nvmem_cell_put(struct nvmem_cell *cell)
{
	struct nvmem_device *nvmem = cell->entry->nvmem;

	if (cell->id)
		kfree_const(cell->id);

	kfree(cell);
	__nvmem_device_put(nvmem);
}
EXPORT_SYMBOL_GPL(nvmem_cell_put);

static void nvmem_shift_read_buffer_in_place(struct nvmem_cell_entry *cell, void *buf)
{
	u8 *p, *b;
	int i, extra, bit_offset = cell->bit_offset;

	p = b = buf;
	if (bit_offset) {
		/* First shift */
		*b++ >>= bit_offset;

		/* setup rest of the bytes if any */
		for (i = 1; i < cell->bytes; i++) {
			/* Get bits from next byte and shift them towards msb */
			*p |= *b << (BITS_PER_BYTE - bit_offset);

			p = b;
			*b++ >>= bit_offset;
		}
	} else {
		/* point to the msb */
		p += cell->bytes - 1;
	}

	/* result fits in less bytes */
	extra = cell->bytes - DIV_ROUND_UP(cell->nbits, BITS_PER_BYTE);
	while (--extra >= 0)
		*p-- = 0;

	/* clear msb bits if any leftover in the last byte */
	if (cell->nbits % BITS_PER_BYTE)
		*p &= GENMASK((cell->nbits % BITS_PER_BYTE) - 1, 0);
}

static int __nvmem_cell_read(struct nvmem_device *nvmem,
			     struct nvmem_cell_entry *cell,
			     void *buf, size_t *len, const char *id, int index)
{
	int rc;

	rc = nvmem->reg_read(nvmem->priv, cell->offset, buf, cell->raw_len);
	if (rc < 0)
		return rc;

	/* shift bits in-place */
	if (cell->bit_offset || cell->nbits)
		nvmem_shift_read_buffer_in_place(cell, buf);

	if (cell->read_post_process) {
		rc = cell->read_post_process(cell->priv, id, index,
				cell->offset, buf, cell->bytes);
		if (rc)
			return rc;
	}

	if (len)
		*len = cell->bytes;

	return 0;
}

/**
 * nvmem_cell_read() - Read a given nvmem cell
 *
 * @cell: nvmem cell to be read.
 * @len: pointer to length of cell which will be populated on successful read;
 *	 can be NULL.
 *
 * Return: ERR_PTR() on error or a valid pointer to a buffer on success. The
 * buffer should be freed by the consumer with a kfree().
 */
void *nvmem_cell_read(struct nvmem_cell *cell, size_t *len)
{
	struct nvmem_cell_entry *entry = cell->entry;
	struct nvmem_device *nvmem = entry->nvmem;
	u8 *buf;
	int rc;

	if (!nvmem)
		return ERR_PTR(-EINVAL);

	buf = kzalloc(max_t(size_t, entry->raw_len, entry->bytes), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	rc = __nvmem_cell_read(nvmem, cell->entry, buf, len, cell->id, cell->index);
	if (rc) {
		kfree(buf);
		return ERR_PTR(rc);
	}

	return buf;
}
EXPORT_SYMBOL_GPL(nvmem_cell_read);

static inline const void *nvmem_cell_prepare_write_buffer(struct nvmem_cell_entry *cell,
							  const u8 *_buf, int len)
{
	struct nvmem_device *nvmem = cell->nvmem;
	int i, rc, nbits, bit_offset = cell->bit_offset;
	u8 v, *p, *buf, *b, pbyte, pbits;

	nbits = cell->nbits;
	buf = kzalloc(cell->bytes, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	memcpy(buf, _buf, len);
	p = b = buf;

	if (bit_offset) {
		pbyte = *b;
		*b <<= bit_offset;

		/* setup the first byte with lsb bits from nvmem */
		rc = nvmem->reg_read(nvmem->priv, cell->offset, &v, 1);
		if (rc < 0)
			return ERR_PTR(rc);

		*b++ |= GENMASK(bit_offset - 1, 0) & v;

		/* setup rest of the byte if any */
		for (i = 1; i < cell->bytes; i++) {
			/* Get last byte bits and shift them towards lsb */
			pbits = pbyte >> (BITS_PER_BYTE - 1 - bit_offset);
			pbyte = *b;
			p = b;
			*b <<= bit_offset;
			*b++ |= pbits;
		}
	}

	/* if it's not end on byte boundary */
	if ((nbits + bit_offset) % BITS_PER_BYTE) {
		/* setup the last byte with msb bits from nvmem */
		rc = nvmem->reg_read(nvmem->priv, cell->offset + cell->bytes - 1,
				      &v, 1);
		if (rc < 0)
			return ERR_PTR(rc);

		*p |= GENMASK(7, (nbits + bit_offset) % BITS_PER_BYTE) & v;

	}

	return buf;
}

static int __nvmem_cell_entry_write(struct nvmem_cell_entry *cell, const void *buf, size_t len)
{
	struct nvmem_device *nvmem = cell->nvmem;
	int rc;

	if (!nvmem || nvmem->read_only ||
	    (cell->bit_offset == 0 && len != cell->bytes))
		return -EINVAL;

	/*
	 * Any cells which have a read_post_process hook are read-only because
	 * we cannot reverse the operation and it might affect other cells,
	 * too.
	 */
	if (cell->read_post_process)
		return -EINVAL;

	if (cell->bit_offset || cell->nbits) {
		buf = nvmem_cell_prepare_write_buffer(cell, buf, len);
		if (IS_ERR(buf))
			return PTR_ERR(buf);
	}

	rc = nvmem->reg_write(nvmem->priv, cell->offset, buf, cell->bytes);

	/* free the tmp buffer */
	if (cell->bit_offset || cell->nbits)
		kfree(buf);

	if (rc)
		return rc;

	return len;
}

/**
 * nvmem_cell_write() - Write to a given nvmem cell
 *
 * @cell: nvmem cell to be written.
 * @buf: Buffer to be written.
 * @len: length of buffer to be written to nvmem cell.
 *
 * Return: length of bytes written or negative on failure.
 */
int nvmem_cell_write(struct nvmem_cell *cell, const void *buf, size_t len)
{
	return __nvmem_cell_entry_write(cell->entry, buf, len);
}
EXPORT_SYMBOL_GPL(nvmem_cell_write);

/**
 * nvmem_device_cell_read() - Read a given nvmem device and cell
 *
 * @nvmem: nvmem device to read from.
 * @info: nvmem cell info to be read.
 * @buf: buffer pointer which will be populated on successful read.
 *
 * Return: length of successful bytes read on success and negative
 * error code on error.
 */
ssize_t nvmem_device_cell_read(struct nvmem_device *nvmem,
			   struct nvmem_cell_info *info, void *buf)
{
	struct nvmem_cell_entry cell;
	int rc;
	ssize_t len = 0;

	if (!nvmem)
		return -EINVAL;

	rc = nvmem_cell_info_to_nvmem_cell_entry_nodup(nvmem, info, &cell);
	if (rc)
		return rc;

	rc = __nvmem_cell_read(nvmem, &cell, buf, &len, NULL, 0);
	if (rc)
		return rc;

	return len;
}
EXPORT_SYMBOL_GPL(nvmem_device_cell_read);

/**
 * nvmem_device_cell_write() - Write cell to a given nvmem device
 *
 * @nvmem: nvmem device to be written to.
 * @info: nvmem cell info to be written
 * @buf: buffer to be written to cell.
 *
 * Return: length of bytes written or negative error code on failure.
 * */
int nvmem_device_cell_write(struct nvmem_device *nvmem,
			    struct nvmem_cell_info *info, const void *buf)
{
	struct nvmem_cell_entry cell;
	int rc;

	if (!nvmem)
		return -EINVAL;

	rc = nvmem_cell_info_to_nvmem_cell_entry_nodup(nvmem, info, &cell);
	if (rc < 0)
		return rc;

	return __nvmem_cell_entry_write(&cell, buf, cell.bytes);
}
EXPORT_SYMBOL_GPL(nvmem_device_cell_write);

/**
 * nvmem_device_read() - Read from a given nvmem device
 *
 * @nvmem: nvmem device to read from.
 * @offset: offset in nvmem device.
 * @bytes: number of bytes to read.
 * @buf: buffer pointer which will be populated on successful read.
 *
 * Return: length of successful bytes read on success and negative
 * error code on error.
 */
int nvmem_device_read(struct nvmem_device *nvmem,
		      unsigned int offset,
		      size_t bytes, void *buf)
{
	int rc;

	if (!nvmem)
		return -EINVAL;

	if (offset >= nvmem->size || bytes > nvmem->size - offset)
		return -EINVAL;

	if (!bytes)
		return 0;

	rc = nvmem->reg_read(nvmem->priv, offset, buf, bytes);

	if (rc < 0)
		return rc;

	return bytes;
}
EXPORT_SYMBOL_GPL(nvmem_device_read);

/**
 * nvmem_device_write() - Write cell to a given nvmem device
 *
 * @nvmem: nvmem device to be written to.
 * @offset: offset in nvmem device.
 * @bytes: number of bytes to write.
 * @buf: buffer to be written.
 *
 * Return: length of bytes written or negative error code on failure.
 * */
int nvmem_device_write(struct nvmem_device *nvmem,
		       unsigned int offset,
		       size_t bytes, const void *buf)
{
	int rc;

	if (!nvmem || nvmem->read_only)
		return -EINVAL;

	if (offset >= nvmem->size || bytes > nvmem->size - offset)
		return -EINVAL;

	if (!bytes)
		return 0;

	rc = nvmem->reg_write(nvmem->priv, offset, buf, bytes);

	if (rc < 0)
		return rc;


	return bytes;
}
EXPORT_SYMBOL_GPL(nvmem_device_write);

ssize_t nvmem_device_size(struct nvmem_device *nvmem)
{
	return nvmem->size;
}
EXPORT_SYMBOL_GPL(nvmem_device_size);

void *nvmem_cell_get_and_read(struct device_node *np, const char *cell_name,
			      size_t bytes)
{
	struct nvmem_cell *cell;
	void *value;
	size_t len;

	cell = of_nvmem_cell_get(np, cell_name);
	if (IS_ERR(cell))
		return cell;

	value = nvmem_cell_read(cell, &len);
	if (!IS_ERR(value) && len != bytes) {
		kfree(value);
		value = ERR_PTR(-EINVAL);
	}

	nvmem_cell_put(cell);

	return value;
}
EXPORT_SYMBOL_GPL(nvmem_cell_get_and_read);

/**
 * nvmem_cell_read_variable_le_u32() - Read up to 32-bits of data as a little endian number.
 *
 * @dev: Device that requests the nvmem cell.
 * @cell_id: Name of nvmem cell to read.
 * @val: pointer to output value.
 *
 * Return: 0 on success or negative errno.
 */
int nvmem_cell_read_variable_le_u32(struct device *dev, const char *cell_id,
				    u32 *val)
{
	size_t len;
	u8 *buf;
	int i;

	len = sizeof(*val);

	buf = nvmem_cell_get_and_read(dev->of_node, cell_id, len);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	/* Copy w/ implicit endian conversion */
	*val = 0;
	for (i = 0; i < len; i++)
		*val |= buf[i] << (8 * i);

	kfree(buf);

	return 0;
}
EXPORT_SYMBOL_GPL(nvmem_cell_read_variable_le_u32);

struct device *nvmem_device_get_device(struct nvmem_device *nvmem)
{
	return &nvmem->dev;
}
EXPORT_SYMBOL_GPL(nvmem_device_get_device);

static int __init nvmem_init(void)
{
	/* TODO: add support for the NVMEM bus too */
	return nvmem_layout_bus_register();
}
pure_initcall(nvmem_init);
