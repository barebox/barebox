/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __REGMAP_H
#define __REGMAP_H

enum regmap_endian {
	/* Unspecified -> 0 -> Backwards compatible default */
	REGMAP_ENDIAN_DEFAULT = 0,
	REGMAP_ENDIAN_BIG,
	REGMAP_ENDIAN_LITTLE,
	REGMAP_ENDIAN_NATIVE,
};

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

	enum regmap_endian reg_format_endian;
	enum regmap_endian val_format_endian;
};

typedef int (*regmap_hw_reg_read)(void *context, unsigned int reg,
				  unsigned int *val);
typedef int (*regmap_hw_reg_write)(void *context, unsigned int reg,
				   unsigned int val);

struct regmap_bus {
	regmap_hw_reg_write reg_write;
	regmap_hw_reg_read reg_read;
	enum regmap_endian reg_format_endian_default;
	enum regmap_endian val_format_endian_default;
};

struct device_d;
struct device_node;

struct regmap *regmap_init(struct device_d *dev,
			     const struct regmap_bus *bus,
			     void *bus_context,
			     const struct regmap_config *config);

struct clk;

/**
 * regmap_init_mmio_clk() - Initialise register map with register clock
 *
 * @dev: Device that will be interacted with
 * @clk_id: register clock consumer ID
 * @regs: Pointer to memory-mapped IO region
 * @config: Configuration for register map
 *
 * The return value will be an ERR_PTR() on error or a valid pointer to
 * a struct regmap.
 */
struct regmap *regmap_init_mmio_clk(struct device_d *dev, const char *clk_id,
				    void __iomem *regs,
				    const struct regmap_config *config);

/**
 * regmap_init_i2c() - Initialise i2c register map
 *
 * @i2c: Device that will be interacted with
 * @config: Configuration for register map
 *
 * The return value will be an ERR_PTR() on error or a valid pointer
 * to a struct regmap.
 */
struct i2c_client;
struct regmap *regmap_init_i2c(struct i2c_client *i2c,
			       const struct regmap_config *config);

/**
 * regmap_init_mmio() - Initialise register map
 *
 * @dev: Device that will be interacted with
 * @regs: Pointer to memory-mapped IO region
 * @config: Configuration for register map
 *
 * The return value will be an ERR_PTR() on error or a valid pointer to
 * a struct regmap.
 */
#define regmap_init_mmio(dev, regs, config)		\
	regmap_init_mmio_clk(dev, NULL, regs, config)


int regmap_mmio_attach_clk(struct regmap *map, struct clk *clk);
void regmap_mmio_detach_clk(struct regmap *map);

void regmap_exit(struct regmap *map);

struct regmap *dev_get_regmap(struct device_d *dev, const char *name);
struct device_d *regmap_get_device(struct regmap *map);

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
int regmap_update_bits(struct regmap *map, unsigned int reg,
		       unsigned int mask, unsigned int val);

static inline int regmap_set_bits(struct regmap *map,
				  unsigned int reg, unsigned int bits)
{
	return regmap_update_bits(map, reg, bits, bits);
}

static inline int regmap_clear_bits(struct regmap *map,
				    unsigned int reg, unsigned int bits)
{
	return regmap_update_bits(map, reg, bits, 0);
}

/**
 * regmap_read_poll_timeout - Poll until a condition is met or a timeout occurs
 *
 * @map: Regmap to read from
 * @addr: Address to poll
 * @val: Unsigned integer variable to read the value into
 * @cond: Break condition (usually involving @val)
 * @timeout_us: Timeout in us, 0 means never timeout
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout or the regmap_read
 * error return value in case of a error read. In the two former cases,
 * the last read value at @addr is stored in @val. Must not be called
 * from atomic context if sleep_us or timeout_us are used.
 *
 * This is modelled after the readx_poll_timeout macros in linux/iopoll.h.
 */
#define regmap_read_poll_timeout(map, addr, val, cond, timeout_us) \
({ \
	int __ret, __tmp; \
	__tmp = read_poll_timeout(regmap_read, __ret, __ret || (cond), \
			timeout_us, (map), (addr), &(val)); \
	__ret ?: __tmp; \
})

#endif /* __REGMAP_H */
