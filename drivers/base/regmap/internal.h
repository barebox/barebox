
#include <linux/list.h>

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