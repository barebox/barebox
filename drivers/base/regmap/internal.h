#ifndef REGMAP_INTERNAL_H_
#define REGMAP_INTERNAL_H_

#include <linux/list.h>
#include <driver.h>

struct regmap_bus;

struct regmap {
	struct device_d *dev;
	const struct regmap_bus *bus;
	const char *name;
	void *bus_context;
	struct list_head list;
	int reg_bits;
	int reg_stride;
	int pad_bits;
	int val_bits;
	int val_bytes;
	unsigned int max_register;

	struct cdev cdev;
};

enum regmap_endian regmap_get_val_endian(struct device_d *dev,
					 const struct regmap_bus *bus,
					 const struct regmap_config *config);

#endif
