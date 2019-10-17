/*
 * nvmem framework core.
 *
 * Copyright (C) 2015 Srinivas Kandagatla <srinivas.kandagatla@linaro.org>
 * Copyright (C) 2013 Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <libbb.h>
#include <malloc.h>
#include <of.h>
#include <linux/nvmem-consumer.h>
#include <linux/nvmem-provider.h>

struct nvmem_device {
	const char		*name;
	struct device_d		dev;
	const struct nvmem_bus	*bus;
	struct list_head	node;
	int			stride;
	int			word_size;
	int			ncells;
	int			users;
	size_t			size;
	bool			read_only;
	struct cdev		cdev;
};

struct nvmem_cell {
	const char		*name;
	int			offset;
	int			bytes;
	int			bit_offset;
	int			nbits;
	struct nvmem_device	*nvmem;
	struct list_head	node;
};

static LIST_HEAD(nvmem_cells);
static LIST_HEAD(nvmem_devs);

int nvmem_device_read(struct nvmem_device *nvmem, unsigned int offset,
		      size_t bytes, void *buf);
int nvmem_device_write(struct nvmem_device *nvmem, unsigned int offset,
		       size_t bytes, const void *buf);


static ssize_t nvmem_cdev_read(struct cdev *cdev, void *buf, size_t count,
			       loff_t offset, unsigned long flags)
{
	struct nvmem_device *nvmem;
	ssize_t retlen;

	if (cdev->master)
		nvmem = container_of(cdev->master, struct nvmem_device, cdev);
	else
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

	if (cdev->master)
		nvmem = container_of(cdev->master, struct nvmem_device, cdev);
	else
		nvmem = container_of(cdev, struct nvmem_device, cdev);

	dev_dbg(cdev->dev, "write ofs: 0x%08llx count: 0x%08zx\n",
		offset, count);

	retlen = nvmem_device_write(nvmem, offset, count, buf);

	return retlen;
}

static struct cdev_operations nvmem_chrdev_ops = {
	.read  = nvmem_cdev_read,
	.write  = nvmem_cdev_write,
};

static int nvmem_register_cdev(struct nvmem_device *nvmem, const char *name)
{
	struct device_d *dev = &nvmem->dev;
	struct cdev *cdev = &nvmem->cdev;
	const char *alias;
	int ret;

	alias = of_alias_get(dev->device_node);

	cdev->name = xstrdup(alias ?: name);
	cdev->ops = &nvmem_chrdev_ops;
	cdev->dev = dev;
	cdev->size = nvmem->size;

	ret = devfs_create(cdev);
	if (ret)
		return ret;

	of_parse_partitions(cdev, dev->device_node);
	of_partitions_register_fixup(cdev);

	return 0;
}

static struct nvmem_device *of_nvmem_find(struct device_node *nvmem_np)
{
	struct nvmem_device *dev;

	if (!nvmem_np)
		return NULL;

	list_for_each_entry(dev, &nvmem_devs, node)
		if (dev->dev.device_node->name && !strcmp(dev->dev.device_node->name, nvmem_np->name))
			return dev;

	return NULL;
}

static struct nvmem_cell *nvmem_find_cell(const char *cell_id)
{
	struct nvmem_cell *p;

	list_for_each_entry(p, &nvmem_cells, node)
		if (p && !strcmp(p->name, cell_id))
			return p;

	return NULL;
}

static void nvmem_cell_drop(struct nvmem_cell *cell)
{
	list_del(&cell->node);
	kfree(cell);
}

static void nvmem_cell_add(struct nvmem_cell *cell)
{
	list_add_tail(&cell->node, &nvmem_cells);
}

static int nvmem_cell_info_to_nvmem_cell(struct nvmem_device *nvmem,
				   const struct nvmem_cell_info *info,
				   struct nvmem_cell *cell)
{
	cell->nvmem = nvmem;
	cell->offset = info->offset;
	cell->bytes = info->bytes;
	cell->name = info->name;

	cell->bit_offset = info->bit_offset;
	cell->nbits = info->nbits;

	if (cell->nbits)
		cell->bytes = DIV_ROUND_UP(cell->nbits + cell->bit_offset,
					   BITS_PER_BYTE);

	if (!IS_ALIGNED(cell->offset, nvmem->stride)) {
		dev_err(&nvmem->dev,
			"cell %s unaligned to nvmem stride %d\n",
			cell->name, nvmem->stride);
		return -EINVAL;
	}

	return 0;
}

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
	nvmem->bus = config->bus;
	np = config->dev->device_node;
	nvmem->dev.device_node = np;

	nvmem->read_only = of_property_read_bool(np, "read-only") |
			   config->read_only;

	dev_set_name(&nvmem->dev, config->name);
	nvmem->dev.id = DEVICE_ID_DYNAMIC;

	dev_dbg(&nvmem->dev, "Registering nvmem device %s\n", config->name);

	rval = register_device(&nvmem->dev);
	if (rval) {
		kfree(nvmem);
		return ERR_PTR(rval);
	}

	rval = nvmem_register_cdev(nvmem, config->name);
	if (rval) {
		kfree(nvmem);
		return ERR_PTR(rval);
	}

	list_add_tail(&nvmem->node, &nvmem_devs);

	return nvmem;
}
EXPORT_SYMBOL_GPL(nvmem_register);

static struct nvmem_device *__nvmem_device_get(struct device_node *np,
					       struct nvmem_cell **cellp,
					       const char *cell_id)
{
	struct nvmem_device *nvmem = NULL;

	if (np) {
		nvmem = of_nvmem_find(np);
		if (!nvmem)
			return ERR_PTR(-EPROBE_DEFER);
	} else {
		struct nvmem_cell *cell = nvmem_find_cell(cell_id);

		if (cell) {
			nvmem = cell->nvmem;
			*cellp = cell;
		}

		if (!nvmem)
			return ERR_PTR(-ENOENT);
	}

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
	int index;

	index = of_property_match_string(np, "nvmem-names", id);

	nvmem_np = of_parse_phandle(np, "nvmem", index);
	if (!nvmem_np)
		return ERR_PTR(-EINVAL);

	return __nvmem_device_get(nvmem_np, NULL, NULL);
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
struct nvmem_device *nvmem_device_get(struct device_d *dev, const char *dev_name)
{
	if (dev->device_node) { /* try dt first */
		struct nvmem_device *nvmem;

		nvmem = of_nvmem_device_get(dev->device_node, dev_name);

		if (!IS_ERR(nvmem) || PTR_ERR(nvmem) == -EPROBE_DEFER)
			return nvmem;

	}

	return nvmem_find(dev_name);
}
EXPORT_SYMBOL_GPL(nvmem_device_get);

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

static struct nvmem_cell *nvmem_cell_get_from_list(const char *cell_id)
{
	struct nvmem_cell *cell = NULL;
	struct nvmem_device *nvmem;

	nvmem = __nvmem_device_get(NULL, &cell, cell_id);
	if (IS_ERR(nvmem))
		return ERR_CAST(nvmem);

	return cell;
}

#if IS_ENABLED(CONFIG_NVMEM) && IS_ENABLED(CONFIG_OFTREE)
/**
 * of_nvmem_cell_get() - Get a nvmem cell from given device node and cell id
 *
 * @dev node: Device tree node that uses the nvmem cell
 * @id: nvmem cell name from nvmem-cell-names property.
 *
 * Return: Will be an ERR_PTR() on error or a valid pointer
 * to a struct nvmem_cell.  The nvmem_cell will be freed by the
 * nvmem_cell_put().
 */
struct nvmem_cell *of_nvmem_cell_get(struct device_node *np,
					    const char *name)
{
	struct device_node *cell_np, *nvmem_np;
	struct nvmem_cell *cell;
	struct nvmem_device *nvmem;
	const __be32 *addr;
	int rval, len, index;

	index = of_property_match_string(np, "nvmem-cell-names", name);

	cell_np = of_parse_phandle(np, "nvmem-cells", index);
	if (!cell_np)
		return ERR_PTR(-EINVAL);

	nvmem_np = of_get_parent(cell_np);
	if (!nvmem_np)
		return ERR_PTR(-EINVAL);

	nvmem = __nvmem_device_get(nvmem_np, NULL, NULL);
	if (IS_ERR(nvmem))
		return ERR_CAST(nvmem);

	addr = of_get_property(cell_np, "reg", &len);
	if (!addr || (len < 2 * sizeof(u32))) {
		dev_err(&nvmem->dev, "nvmem: invalid reg on %s\n",
			cell_np->full_name);
		rval  = -EINVAL;
		goto err_mem;
	}

	cell = kzalloc(sizeof(*cell), GFP_KERNEL);
	if (!cell) {
		rval = -ENOMEM;
		goto err_mem;
	}

	cell->nvmem = nvmem;
	cell->offset = be32_to_cpup(addr++);
	cell->bytes = be32_to_cpup(addr);
	cell->name = cell_np->name;

	addr = of_get_property(cell_np, "bits", &len);
	if (addr && len == (2 * sizeof(u32))) {
		cell->bit_offset = be32_to_cpup(addr++);
		cell->nbits = be32_to_cpup(addr);
	}

	if (cell->nbits)
		cell->bytes = DIV_ROUND_UP(cell->nbits + cell->bit_offset,
					   BITS_PER_BYTE);

	if (cell->bytes < nvmem->word_size)
		cell->bytes = nvmem->word_size;

	if (!IS_ALIGNED(cell->offset, nvmem->stride)) {
			dev_err(&nvmem->dev,
				"cell %s unaligned to nvmem stride %d\n",
				cell->name, nvmem->stride);
		rval  = -EINVAL;
		goto err_sanity;
	}

	nvmem_cell_add(cell);

	return cell;

err_sanity:
	kfree(cell);

err_mem:
	__nvmem_device_put(nvmem);

	return ERR_PTR(rval);
}
EXPORT_SYMBOL_GPL(of_nvmem_cell_get);
#endif

/**
 * nvmem_cell_get() - Get nvmem cell of device form a given cell name
 *
 * @dev node: Device tree node that uses the nvmem cell
 * @id: nvmem cell name to get.
 *
 * Return: Will be an ERR_PTR() on error or a valid pointer
 * to a struct nvmem_cell.  The nvmem_cell will be freed by the
 * nvmem_cell_put().
 */
struct nvmem_cell *nvmem_cell_get(struct device_d *dev, const char *cell_id)
{
	struct nvmem_cell *cell;

	if (dev->device_node) { /* try dt first */
		cell = of_nvmem_cell_get(dev->device_node, cell_id);
		if (!IS_ERR(cell) || PTR_ERR(cell) == -EPROBE_DEFER)
			return cell;
	}

	return nvmem_cell_get_from_list(cell_id);
}
EXPORT_SYMBOL_GPL(nvmem_cell_get);

/**
 * nvmem_cell_put() - Release previously allocated nvmem cell.
 *
 * @cell: Previously allocated nvmem cell by nvmem_cell_get()
 */
void nvmem_cell_put(struct nvmem_cell *cell)
{
	struct nvmem_device *nvmem = cell->nvmem;

	__nvmem_device_put(nvmem);
	nvmem_cell_drop(cell);
}
EXPORT_SYMBOL_GPL(nvmem_cell_put);

static inline void nvmem_shift_read_buffer_in_place(struct nvmem_cell *cell,
						    void *buf)
{
	u8 *p, *b;
	int i, bit_offset = cell->bit_offset;

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

		/* result fits in less bytes */
		if (cell->bytes != DIV_ROUND_UP(cell->nbits, BITS_PER_BYTE))
			*p-- = 0;
	}
	/* clear msb bits if any leftover in the last byte */
	*p &= GENMASK((cell->nbits%BITS_PER_BYTE) - 1, 0);
}

static int __nvmem_cell_read(struct nvmem_device *nvmem,
		      struct nvmem_cell *cell,
		      void *buf, size_t *len)
{
	int rc;

	rc = nvmem->bus->read(&nvmem->dev, cell->offset, buf, cell->bytes);
	if (IS_ERR_VALUE(rc))
		return rc;

	/* shift bits in-place */
	if (cell->bit_offset || cell->nbits)
		nvmem_shift_read_buffer_in_place(cell, buf);

	*len = cell->bytes;

	return 0;
}

/**
 * nvmem_cell_read() - Read a given nvmem cell
 *
 * @cell: nvmem cell to be read.
 * @len: pointer to length of cell which will be populated on successful read.
 *
 * Return: ERR_PTR() on error or a valid pointer to a char * buffer on success.
 * The buffer should be freed by the consumer with a kfree().
 */
void *nvmem_cell_read(struct nvmem_cell *cell, size_t *len)
{
	struct nvmem_device *nvmem = cell->nvmem;
	u8 *buf;
	int rc;

	if (!nvmem)
		return ERR_PTR(-EINVAL);

	buf = kzalloc(cell->bytes, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	rc = __nvmem_cell_read(nvmem, cell, buf, len);
	if (IS_ERR_VALUE(rc)) {
		kfree(buf);
		return ERR_PTR(rc);
	}

	return buf;
}
EXPORT_SYMBOL_GPL(nvmem_cell_read);

static inline void *nvmem_cell_prepare_write_buffer(struct nvmem_cell *cell,
						    u8 *_buf, int len)
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
		rc = nvmem->bus->read(&nvmem->dev, cell->offset, &v, 1);
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
		rc = nvmem->bus->read(&nvmem->dev, cell->offset + cell->bytes - 1,
				      &v, 1);
		*p |= GENMASK(7, (nbits + bit_offset) % BITS_PER_BYTE) & v;

	}

	return buf;
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
int nvmem_cell_write(struct nvmem_cell *cell, void *buf, size_t len)
{
	struct nvmem_device *nvmem = cell->nvmem;
	int rc;

	if (!nvmem || nvmem->read_only ||
	    (cell->bit_offset == 0 && len != cell->bytes))
		return -EINVAL;

	if (cell->bit_offset || cell->nbits) {
		buf = nvmem_cell_prepare_write_buffer(cell, buf, len);
		if (IS_ERR(buf))
			return PTR_ERR(buf);
	}

	rc = nvmem->bus->write(&nvmem->dev, cell->offset, buf, cell->bytes);

	/* free the tmp buffer */
	if (cell->bit_offset || cell->nbits)
		kfree(buf);

	if (IS_ERR_VALUE(rc))
		return rc;

	return len;
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
	struct nvmem_cell cell;
	int rc;
	ssize_t len;

	if (!nvmem)
		return -EINVAL;

	rc = nvmem_cell_info_to_nvmem_cell(nvmem, info, &cell);
	if (IS_ERR_VALUE(rc))
		return rc;

	rc = __nvmem_cell_read(nvmem, &cell, buf, &len);
	if (IS_ERR_VALUE(rc))
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
			    struct nvmem_cell_info *info, void *buf)
{
	struct nvmem_cell cell;
	int rc;

	if (!nvmem)
		return -EINVAL;

	rc = nvmem_cell_info_to_nvmem_cell(nvmem, info, &cell);
	if (IS_ERR_VALUE(rc))
		return rc;

	return nvmem_cell_write(&cell, buf, cell.bytes);
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

	rc = nvmem->bus->read(&nvmem->dev, offset, buf, bytes);

	if (IS_ERR_VALUE(rc))
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

	rc = nvmem->bus->write(&nvmem->dev, offset, buf, bytes);

	if (IS_ERR_VALUE(rc))
		return rc;


	return bytes;
}
EXPORT_SYMBOL_GPL(nvmem_device_write);

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
