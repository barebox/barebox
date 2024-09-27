// SPDX-License-Identifier: GPL-2.0-only
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
 */

#include <common.h>
#include <linux/regmap.h>
#include <malloc.h>
#include <linux/log2.h>

#include "internal.h"

static LIST_HEAD(regmaps);

enum regmap_endian regmap_get_val_endian(struct device *dev,
					 const struct regmap_bus *bus,
					 const struct regmap_config *config)
{
	struct device_node *np;
	enum regmap_endian endian;

	/* Retrieve the endianness specification from the regmap config */
	endian = config->val_format_endian;

	/* If the regmap config specified a non-default value, use that */
	if (endian != REGMAP_ENDIAN_DEFAULT)
		return endian;

	/* If the dev and dev->device_node exist try to get endianness from DT */
	if (dev && dev->of_node) {
		np = dev->of_node;

		/* Parse the device's DT node for an endianness specification */
		if (of_property_read_bool(np, "big-endian"))
			endian = REGMAP_ENDIAN_BIG;
		else if (of_property_read_bool(np, "little-endian"))
			endian = REGMAP_ENDIAN_LITTLE;
		else if (of_property_read_bool(np, "native-endian"))
			endian = REGMAP_ENDIAN_NATIVE;

		/* If the endianness was specified in DT, use that */
		if (endian != REGMAP_ENDIAN_DEFAULT)
			return endian;
	}

	/* Retrieve the endianness specification from the bus config */
	if (bus && bus->val_format_endian_default)
		endian = bus->val_format_endian_default;

	/* If the bus specified a non-default value, use that */
	if (endian != REGMAP_ENDIAN_DEFAULT)
		return endian;

	/* Use this if no other value was found */
	return REGMAP_ENDIAN_BIG;
}
EXPORT_SYMBOL_GPL(regmap_get_val_endian);

static int _regmap_bus_reg_read(void *context, unsigned int reg,
				unsigned int *val)
{
	struct regmap *map = context;

	return map->bus->reg_read(map->bus_context, reg, val);
}


static int _regmap_bus_reg_write(void *context, unsigned int reg,
				 unsigned int val)
{
	struct regmap *map = context;

	return map->bus->reg_write(map->bus_context, reg, val);
}

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
struct regmap *regmap_init(struct device *dev,
			     const struct regmap_bus *bus,
			     void *bus_context,
			     const struct regmap_config *config)
{
	struct regmap *map;
	int ret;

	map = xzalloc(sizeof(*map));
	map->dev = dev;
	map->bus = bus;
	map->name = config->name;
	map->bus_context = bus_context;
	map->format.reg_bytes = DIV_ROUND_UP(config->reg_bits, 8);
	map->reg_stride = config->reg_stride;
	if (!map->reg_stride)
		map->reg_stride = 1;
	map->format.pad_bytes = config->pad_bits / 8;
	map->format.val_bytes = DIV_ROUND_UP(config->val_bits, 8);
	map->reg_shift = config->pad_bits % 8;
	map->max_register = config->max_register;

	if (!bus->read || !bus->write) {
		map->reg_read = _regmap_bus_reg_read;
		map->reg_write = _regmap_bus_reg_write;
	} else  {
		ret = regmap_formatted_init(map, config);
		if (ret) {
			free(map);
			return ERR_PTR(ret);
		}
	}

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
struct regmap *dev_get_regmap(struct device *dev, const char *name)
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

struct device *regmap_get_device(struct regmap *map)
{
	return map->dev;
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
	return map->reg_write(map, reg, val);
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
	return map->reg_read(map, reg, val);
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
 * regmap_field_read(): Read a value to a single register field
 *
 * @field: Register field to read from
 * @val: Pointer to store read value
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
int regmap_field_read(struct regmap_field *field, unsigned int *val)
{
	int ret;
	unsigned int reg_val;
	ret = regmap_read(field->regmap, field->reg, &reg_val);
	if (ret != 0)
		return ret;

	reg_val &= field->mask;
	reg_val >>= field->shift;
	*val = reg_val;

	return ret;
}

/**
 * regmap_bulk_read(): Read data from the device
 *
 * @map: Register map to read from
 * @reg: First register to be read from
 * @val: Pointer to store read value
 * @val_count: Number of registers to read
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
int regmap_bulk_read(struct regmap *map, unsigned int reg, void *val,
		     size_t val_count)
{
	unsigned int v;
	int ret, i;

	if (!IS_ALIGNED(reg, map->reg_stride))
		return -EINVAL;
	if (val_count == 0)
		return -EINVAL;

	for (i = 0; i < val_count; i++) {

#ifdef CONFIG_64BIT
		u64 *u64 = val;
#endif
		u32 *u32 = val;
		u16 *u16 = val;
		u8 *u8 = val;

		ret = regmap_read(map, reg + (i * map->reg_stride), &v);
		if (ret != 0)
			goto out;

		switch (map->format.val_bytes) {
#ifdef CONFIG_64BIT
		case 8:
			u64[i] = v;
			break;
#endif
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
 * @val_len: Number of registers to write
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
int regmap_bulk_write(struct regmap *map, unsigned int reg,
		     const void *val, size_t val_count)
{
	size_t val_bytes = map->format.val_bytes;
	int ret, i;

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
#ifdef CONFIG_64BIT
		case 8:
			ival = *(u64 *)(val + (i * val_bytes));
			break;
#endif
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
	return map->format.val_bytes;
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
	return map->format.val_bytes ?: 1;
}

/**
 * regmap_update_bits_base() - Perform a read/modify/write cycle on a register
 *
 * @map: Register map to update
 * @reg: Register to update
 * @mask: Bitmask to change
 * @val: New value for bitmask
 * @change: Boolean indicating if a write was done
 * @async: Boolean indicating asynchronously
 * @force: Boolean indicating use force update
 *
 * Perform a read/modify/write cycle on a register map with change, async, force
 * options.
 *
 * If async is true:
 *
 * With most buses the read must be done synchronously so this is most useful
 * for devices with a cache which do not need to interact with the hardware to
 * determine the current register value.
 *
 * Returns zero for success, a negative number on error.
 */
int regmap_update_bits_base(struct regmap *map, unsigned int reg,
			    unsigned int mask, unsigned int val,
			    bool *change, bool async, bool force)
{
	int ret;

	ret = regmap_update_bits(map, reg, mask, val);

	return ret;
}

/**
 * regmap_field_update_bits_base() - Perform a read/modify/write cycle a
 *                                   register field.
 *
 * @field: Register field to write to
 * @mask: Bitmask to change
 * @val: Value to be written
 * @change: Boolean indicating if a write was done
 * @async: Boolean indicating asynchronously
 * @force: Boolean indicating use force update
 *
 * Perform a read/modify/write cycle on the register field with change,
 * async, force option.
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
int regmap_field_update_bits_base(struct regmap_field *field,
				  unsigned int mask, unsigned int val,
				  bool *change, bool async, bool force)
{
	mask = (mask << field->shift) & field->mask;

	return regmap_update_bits_base(field->regmap, field->reg,
				       mask, val << field->shift,
				       change, async, force);
}

static ssize_t regmap_cdev_read(struct cdev *cdev, void *buf, size_t count, loff_t offset,
		       unsigned long flags)
{
	struct regmap *map = container_of(cdev, struct regmap, cdev);
	size_t val_bytes = map->format.val_bytes;
	int ret;

	count = ALIGN_DOWN(count, val_bytes);
	ret = regmap_bulk_read(map, offset, buf, count / val_bytes);
	if (ret)
		return ret;

	return count;
}

static ssize_t regmap_cdev_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset,
			unsigned long flags)
{
	struct regmap *map = container_of(cdev, struct regmap, cdev);
	size_t val_bytes = map->format.val_bytes;
	int ret;

	count = ALIGN_DOWN(count, val_bytes);
	ret = regmap_bulk_write(map, offset, buf, count / val_bytes);
	if (ret)
		return ret;

	return count;
}

static struct cdev_operations regmap_fops = {
	.read	= regmap_cdev_read,
	.write	= regmap_cdev_write,
};

/*
 * regmap_count_registers - returns the total number of registers
 *
 * @map:	The map
 *
 * Returns the total number of registers in a regmap
 */
static size_t regmap_count_registers(struct regmap *map)
{
	/*
	 * max_register is in units of reg_stride, so we need to divide
	 * by the register stride before adding one to arrive at the
	 * total number of registers.
	 */
	return (map->max_register / map->reg_stride) + 1;
}

/*
 * regmap_size_bytes - computes the size of the regmap in bytes
 *
 * @map:	The map
 *
 * Returns the number of bytes needed to hold all values in the
 * regmap.
 */
size_t regmap_size_bytes(struct regmap *map)
{
	return regmap_round_val_bytes(map) * regmap_count_registers(map);
}

static void regmap_field_init(struct regmap_field *rm_field,
	struct regmap *regmap, struct reg_field reg_field)
{
	rm_field->regmap = regmap;
	rm_field->reg = reg_field.reg;
	rm_field->shift = reg_field.lsb;
	rm_field->mask = GENMASK(reg_field.msb, reg_field.lsb);

	WARN_ONCE(rm_field->mask == 0, "invalid empty mask defined\n");

	rm_field->id_size = reg_field.id_size;
	rm_field->id_offset = reg_field.id_offset;
}

/**
 * regmap_field_bulk_alloc() - Allocate and initialise a bulk register field.
 *
 * @regmap: regmap bank in which this register field is located.
 * @rm_field: regmap register fields within the bank.
 * @reg_field: Register fields within the bank.
 * @num_fields: Number of register fields.
 *
 * The return value will be an -ENOMEM on error or zero for success.
 * Newly allocated regmap_fields should be freed by calling
 * regmap_field_bulk_free()
 */
int regmap_field_bulk_alloc(struct regmap *regmap,
			    struct regmap_field **rm_field,
			    const struct reg_field *reg_field,
			    int num_fields)
{
	struct regmap_field *rf;
	int i;

	rf = kcalloc(num_fields, sizeof(*rf), GFP_KERNEL);
	if (!rf)
		return -ENOMEM;

	for (i = 0; i < num_fields; i++) {
		regmap_field_init(&rf[i], regmap, reg_field[i]);
		rm_field[i] = &rf[i];
	}

	return 0;
}

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

	map->cdev.size = regmap_size_bytes(map);
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
