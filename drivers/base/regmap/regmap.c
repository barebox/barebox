/*
 * Register map access API
 *
 * Copyright 2016 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * based on Kernel code:
 *
 * Copyright 2011 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <regmap.h>
#include <malloc.h>
#include <linux/log2.h>

#include "internal.h"

static LIST_HEAD(regmaps);

/*
 * regmap_init - initialize and register a regmap
 *
 * @dev:         The device the new regmap belongs to
 * @bus:         The regmap_bus providing read/write access to the map
 * @bus_context: Context pointer for the bus ops
 * @config:      Configuration options for this map
 *
 * Returns a pointer to the new map or a ERR_PTR value on failure
 */
struct regmap *regmap_init(struct device_d *dev,
			     const struct regmap_bus *bus,
			     void *bus_context,
			     const struct regmap_config *config)
{
	struct regmap *map;

	map = xzalloc(sizeof(*map));
	map->dev = dev;
	map->bus = bus;
	map->name = config->name;
	map->bus_context = bus_context;
	map->reg_bits = config->reg_bits;
	map->reg_stride = config->reg_stride;
	if (!map->reg_stride)
		map->reg_stride = 1;
	map->pad_bits = config->pad_bits;
	map->val_bits = config->val_bits;
	map->val_bytes = DIV_ROUND_UP(config->val_bits, 8);
	map->max_register = config->max_register;

	list_add_tail(&map->list, &regmaps);

	return map;
}

/*
 * dev_get_regmap - get a regmap from a device
 *
 * @dev:         The device the maps is attached to
 * @name:        Optional name for the map. If given it must match.
 *
 * Returns a pointer to the regmap or a ERR_PTR value on failure
 */
struct regmap *dev_get_regmap(struct device_d *dev, const char *name)
{
	struct regmap *map;

	list_for_each_entry(map, &regmaps, list) {
		if (map->dev != dev)
			continue;
		if (!name)
			return map;
		if (!strcmp(map->name, name))
			return map;
	}

	return ERR_PTR(-ENOENT);
}

/*
 * of_node_to_regmap - get a regmap from a device node
 *
 * node:         The device node
 *
 * Returns a pointer to the regmap or a ERR_PTR if the node has no
 * regmap attached.
 */
struct regmap *of_node_to_regmap(struct device_node *node)
{
	struct regmap *map;

	list_for_each_entry(map, &regmaps, list) {
		if (map->dev->device_node == node)
			return map;
	}

	return ERR_PTR(-ENOENT);
}

/*
 * regmap_write - write a register in a map
 *
 * @map:	The map
 * @reg:	The register offset of the register
 * @val:	The value to be written
 *
 * Returns 0 for success or negative error code on failure
 */
int regmap_write(struct regmap *map, unsigned int reg, unsigned int val)
{
	return map->bus->reg_write(map->bus_context, reg, val);
}

/*
 * regmap_write - read a register from a map
 *
 * @map:	The map
 * @reg:	The register offset of the register
 * @val:	pointer to value read
 *
 * Returns 0 for success or negative error code on failure
 */
int regmap_read(struct regmap *map, unsigned int reg, unsigned int *val)
{
	return map->bus->reg_read(map->bus_context, reg, val);
}

/**
 * regmap_update_bits() - Perform a read/modify/write cycle on a register
 *
 * @map: Register map to update
 * @reg: Register to update
 * @mask: Bitmask to change
 * @val: New value for bitmask
 *
 * Returns zero for success, a negative number on error.
 */
int regmap_update_bits(struct regmap *map, unsigned int reg,
		       unsigned int mask, unsigned int val)
{
	int ret;
	unsigned int tmp, orig;

	ret = regmap_read(map, reg, &orig);
	if (ret != 0)
		return ret;

	tmp = orig & ~mask;
	tmp |= val & mask;

	if (tmp != orig)
		ret = regmap_write(map, reg, tmp);

	return ret;
}

/**
 * regmap_write_bits - write bits of a register in a map
 *
 * @map:	The map
 * @reg:	The register offset of the register
 * @mask:	Mask indicating bits to be modified
 *		(1 - modified, 0 - untouched)
 * @val:	Bit value to be set
 *
 * Returns 0 for success or negative error code on failure
 */
int regmap_write_bits(struct regmap *map, unsigned int reg,
		      unsigned int mask, unsigned int val)
{
	int ret;
	unsigned int tmp, orig;

	ret = regmap_read(map, reg, &orig);
	if (ret != 0)
		return ret;

	tmp = orig & ~mask;
	tmp |= val & mask;

	return regmap_write(map, reg, tmp);
}

/**
 * regmap_bulk_read(): Read data from the device
 *
 * @map: Register map to read from
 * @reg: First register to be read from
 * @val: Pointer to store read value
 * @val_len: Size of data to read
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
int regmap_bulk_read(struct regmap *map, unsigned int reg, void *val,
		     size_t val_len)
{
	size_t val_bytes = map->val_bytes;
	size_t val_count = val_len / val_bytes;
	unsigned int v;
	int ret, i;

	if (val_len % val_bytes)
		return -EINVAL;
	if (!IS_ALIGNED(reg, map->reg_stride))
		return -EINVAL;
	if (val_count == 0)
		return -EINVAL;

	for (i = 0; i < val_count; i++) {
		u32 *u32 = val;
		u16 *u16 = val;
		u8 *u8 = val;

		ret = regmap_read(map, reg + (i * map->reg_stride), &v);
		if (ret != 0)
			goto out;

		switch (map->val_bytes) {
		case 4:
			u32[i] = v;
			break;
		case 2:
			u16[i] = v;
			break;
		case 1:
			u8[i] = v;
			break;
		default:
			return -EINVAL;
		}
	}

 out:
	return ret;
}

/**
 * regmap_bulk_write(): Write values to one or more registers
 *
 * @map: Register map to write to
 * @reg: Initial register to write to
 * @val: Block of data to be written, laid out for direct transmission to the
 *       device
 * @val_len: Length of data pointed to by val.
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
int regmap_bulk_write(struct regmap *map, unsigned int reg,
		     const void *val, size_t val_len)
{
	size_t val_bytes = map->val_bytes;
	size_t val_count = val_len / val_bytes;
	int ret, i;

	if (val_len % val_bytes)
		return -EINVAL;
	if (!IS_ALIGNED(reg, map->reg_stride))
		return -EINVAL;
	if (val_count == 0)
		return -EINVAL;

	for (i = 0; i < val_count; i++) {
		unsigned int ival;

		switch (val_bytes) {
		case 1:
			ival = *(u8 *)(val + (i * val_bytes));
			break;
		case 2:
			ival = *(u16 *)(val + (i * val_bytes));
			break;
		case 4:
			ival = *(u32 *)(val + (i * val_bytes));
			break;
		default:
			ret = -EINVAL;
			goto out;
		}

		ret = regmap_write(map, reg + (i * map->reg_stride),
				ival);
		if (ret != 0)
			goto out;
	}

 out:
	return ret;
}

int regmap_get_val_bytes(struct regmap *map)
{
	return map->val_bytes;
}

int regmap_get_max_register(struct regmap *map)
{
	return map->max_register;
}

int regmap_get_reg_stride(struct regmap *map)
{
	return map->reg_stride;
}

static int regmap_round_val_bytes(struct regmap *map)
{
	int val_bytes;

	val_bytes = roundup_pow_of_two(map->val_bits) >> 3;
	if (!val_bytes)
		val_bytes = 1;

	return val_bytes;
}

static ssize_t regmap_cdev_read(struct cdev *cdev, void *buf, size_t count, loff_t offset,
		       unsigned long flags)
{
	struct regmap *map = container_of(cdev, struct regmap, cdev);
	int ret;

	ret = regmap_bulk_read(map, offset, buf, count);
	if (ret)
		return ret;

	return count;
}

static ssize_t regmap_cdev_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset,
			unsigned long flags)
{
	struct regmap *map = container_of(cdev, struct regmap, cdev);
	int ret;

	ret = regmap_bulk_write(map, offset, buf, count);
	if (ret)
		return ret;

	return count;
}

static struct cdev_operations regmap_fops = {
	.read	= regmap_cdev_read,
	.write	= regmap_cdev_write,
};

/*
 * regmap_register_cdev - register a devfs file for a regmap
 *
 * @map:	The map
 * @name:	Optional name of the device file
 *
 * Returns 0 for success or negative error code on failure
 */
int regmap_register_cdev(struct regmap *map, const char *name)
{
	int ret;

	if (map->cdev.name)
		return -EBUSY;

	if (!map->max_register)
		return -EINVAL;

	if (name) {
		map->cdev.name = xstrdup(name);
	} else {
		if (map->name)
			map->cdev.name = xasprintf("%s-%s", dev_name(map->dev), map->name);
		else
			map->cdev.name = xstrdup(dev_name(map->dev));
	}

	map->cdev.size = regmap_round_val_bytes(map) * (map->max_register + 1) /
			map->reg_stride;
	map->cdev.dev = map->dev;
	map->cdev.ops = &regmap_fops;

	ret = devfs_create(&map->cdev);
	if (ret)
		return ret;

	return 0;
}

void regmap_exit(struct regmap *map)
{
	list_del(&map->list);

	if (map->cdev.name) {
		devfs_remove(&map->cdev);
		free(map->cdev.name);
	}

	free(map);
}
