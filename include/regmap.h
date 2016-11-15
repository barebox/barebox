#ifndef __REGMAP_H
#define __REGMAP_H

/**
 * Configuration for the register map of a device.
 *
 * @name: Optional name of the regmap. Useful when a device has multiple
 *        register regions.
 *
 * @reg_bits: Number of bits in a register address, mandatory.
 * @reg_stride: The register address stride. Valid register addresses are a
 *              multiple of this value. If set to 0, a value of 1 will be
 *              used.
 * @pad_bits: Number of bits of padding between register and value.
 * @val_bits: Number of bits in a register value, mandatory.
 *
 * @max_register: Optional, specifies the maximum valid register index.
 */
struct regmap_config {
	const char *name;

	int reg_bits;
	int reg_stride;
	int pad_bits;
	int val_bits;

	unsigned int max_register;
};

typedef int (*regmap_hw_reg_read)(void *context, unsigned int reg,
				  unsigned int *val);
typedef int (*regmap_hw_reg_write)(void *context, unsigned int reg,
				   unsigned int val);

struct regmap_bus {
	regmap_hw_reg_write reg_write;
	regmap_hw_reg_read reg_read;
};

struct regmap *regmap_init(struct device_d *dev,
			     const struct regmap_bus *bus,
			     void *bus_context,
			     const struct regmap_config *config);
void regmap_exit(struct regmap *map);

struct regmap *dev_get_regmap(struct device_d *dev, const char *name);
struct regmap *of_node_to_regmap(struct device_node *node);

int regmap_register_cdev(struct regmap *map, const char *name);

int regmap_write(struct regmap *map, unsigned int reg, unsigned int val);
int regmap_read(struct regmap *map, unsigned int reg, unsigned int *val);

int regmap_bulk_read(struct regmap *map, unsigned int reg, void *val,
		    size_t val_len);
int regmap_bulk_write(struct regmap *map, unsigned int reg,
		     const void *val, size_t val_len);

int regmap_get_val_bytes(struct regmap *map);
int regmap_get_max_register(struct regmap *map);
int regmap_get_reg_stride(struct regmap *map);

int regmap_write_bits(struct regmap *map, unsigned int reg,
		      unsigned int mask, unsigned int val);


#endif /* __REGMAP_H */
