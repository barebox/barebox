/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef REGMAP_INTERNAL_H_
#define REGMAP_INTERNAL_H_

#include <linux/list.h>
#include <driver.h>

struct regmap_bus;

struct regmap_format {
	size_t buf_size;
	size_t reg_bytes;
	size_t pad_bytes;
	size_t val_bytes;
	void (*format_write)(struct regmap *map,
			     unsigned int reg, unsigned int val);
	void (*format_reg)(void *buf, unsigned int reg, unsigned int shift);
	void (*format_val)(void *buf, unsigned int val, unsigned int shift);
	unsigned int (*parse_val)(const void *buf);
};

struct regmap {
	struct device *dev;
	const struct regmap_bus *bus;
	const char *name;
	void *bus_context;
	struct list_head list;
	int reg_stride;
	void *work_buf;     /* Scratch buffer used to format I/O */
	struct regmap_format format;
	unsigned int read_flag_mask;
	unsigned int write_flag_mask;
	int reg_shift;
	unsigned int max_register;

	struct cdev cdev;

	int (*reg_read)(void *context, unsigned int reg,
			unsigned int *val);
	int (*reg_write)(void *context, unsigned int reg,
			 unsigned int val);
};

struct regmap_field {
	struct regmap *regmap;
	unsigned int mask;
	/* lsb */
	unsigned int shift;
	unsigned int reg;

	unsigned int id_size;
	unsigned int id_offset;
};

enum regmap_endian regmap_get_val_endian(struct device *dev,
					 const struct regmap_bus *bus,
					 const struct regmap_config *config);

#ifdef CONFIG_REGMAP_FORMATTED
int regmap_formatted_init(struct regmap *map, const struct regmap_config *);
#else
static inline int regmap_formatted_init(struct regmap *map, const struct regmap_config *cfg)
{
	return -ENOSYS;
}
#endif

#endif
