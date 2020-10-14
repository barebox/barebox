/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __REGULATOR_H
#define __REGULATOR_H

struct device_d;

/* struct regulator is an opaque object for consumers */
struct regulator;

/**
 * struct regulator_bulk_data - Data used for bulk regulator operations.
 *
 * @supply:   The name of the supply.  Initialised by the user before
 *            using the bulk regulator APIs.
 * @consumer: The regulator consumer for the supply.  This will be managed
 *            by the bulk API.
 *
 * The regulator APIs provide a series of regulator_bulk_() API calls as
 * a convenience to consumers which require multiple supplies.  This
 * structure is used to manage data for these calls.
 */
struct regulator_bulk_data {
	const char *supply;
	struct regulator *consumer;
};

/**
 * struct regulator_desc - Static regulator descriptor
 *
 * Each regulator registered with the core is described with a
 * structure of this type and a struct regulator_config.  This
 * structure contains the non-varying parts of the regulator
 * description.
 *
 * @n_voltages: Number of selectors available for ops.list_voltage().
 * @ops: Regulator operations table.
 *
 * @min_uV: Voltage given by the lowest selector (if linear mapping)
 * @uV_step: Voltage increase with each selector (if linear mapping)
 * @linear_min_sel: Minimal selector for starting linear mapping
 *
 * @vsel_reg: Register for selector when using regulator_regmap_X_voltage_
 * @vsel_mask: Mask for register bitfield used for selector
 * @apply_reg: Register for initiate voltage change on the output when
 *                using regulator_set_voltage_sel_regmap
 * @apply_bit: Register bitfield used for initiate voltage change on the
 *                output when using regulator_set_voltage_sel_regmap
 * @enable_reg: Register for control when using regmap enable/disable ops
 * @enable_mask: Mask for control when using regmap enable/disable ops
 * @enable_val: Enabling value for control when using regmap enable/disable ops
 * @disable_val: Disabling value for control when using regmap enable/disable ops
 * @enable_is_inverted: A flag to indicate set enable_mask bits to disable
 *                      when using regulator_enable_regmap and friends APIs.
 */

struct regulator_desc {
	unsigned n_voltages;
	const struct regulator_ops *ops;

	unsigned int min_uV;
	unsigned int uV_step;
	unsigned int linear_min_sel;

	unsigned int vsel_reg;
	unsigned int vsel_mask;
	unsigned int apply_reg;
	unsigned int apply_bit;
	unsigned int enable_reg;
	unsigned int enable_mask;
	unsigned int enable_val;
	unsigned int disable_val;
	bool enable_is_inverted;

	const struct regulator_linear_range *linear_ranges;
	int n_linear_ranges;
};

struct regulator_dev {
	const struct regulator_desc *desc;
	struct regmap *regmap;
	int boot_on;
	/* the device this regulator device belongs to */
	struct device_d *dev;
};

struct regulator_ops {
	/* enable/disable regulator */
	int (*enable) (struct regulator_dev *);
	int (*disable) (struct regulator_dev *);
	int (*is_enabled) (struct regulator_dev *);

	int (*list_voltage) (struct regulator_dev *, unsigned int);
	int (*set_voltage_sel) (struct regulator_dev *, unsigned int);
	int (*map_voltage)(struct regulator_dev *, int min_uV, int max_uV);
};

/*
 * struct regulator_linear_range - specify linear voltage ranges
 *
 * Specify a range of voltages for regulator_map_linear_range() and
 * regulator_list_linear_range().
 *
 * @min_uV:  Lowest voltage in range
 * @min_sel: Lowest selector for range
 * @max_sel: Highest selector for range
 * @uV_step: Step size
 */
struct regulator_linear_range {
	unsigned int min_uV;
	unsigned int min_sel;
	unsigned int max_sel;
	unsigned int uV_step;
};

/* Initialize struct regulator_linear_range */
#define REGULATOR_LINEAR_RANGE(_min_uV, _min_sel, _max_sel, _step_uV)	\
{									\
	.min_uV		= _min_uV,					\
	.min_sel	= _min_sel,					\
	.max_sel	= _max_sel,					\
	.uV_step	= _step_uV,					\
}

#ifdef CONFIG_OFDEVICE
int of_regulator_register(struct regulator_dev *rd, struct device_node *node);
#else
static inline int of_regulator_register(struct regulator_dev *rd,
					struct device_node *node)
{
	return -ENOSYS;
}
#endif
int dev_regulator_register(struct regulator_dev *rd, const char * name,
			   const char* supply);

void regulators_print(void);

#ifdef CONFIG_REGULATOR

struct regulator *regulator_get(struct device_d *, const char *);
struct regulator *regulator_get_name(const char *name);
int regulator_enable(struct regulator *);
int regulator_disable(struct regulator *);
int regulator_is_enabled_regmap(struct regulator_dev *);
int regulator_enable_regmap(struct regulator_dev *);
int regulator_disable_regmap(struct regulator_dev *);
int regulator_set_voltage_sel_regmap(struct regulator_dev *, unsigned);
int regulator_set_voltage(struct regulator *regulator, int min_uV, int max_uV);
int regulator_map_voltage_linear(struct regulator_dev *rdev,
				 int min_uV, int max_uV);
int regulator_map_voltage_linear_range(struct regulator_dev *rdev,
				       int min_uV, int max_uV);
int regulator_list_voltage_linear(struct regulator_dev *rdev,
				  unsigned int selector);
int regulator_list_voltage_linear_range(struct regulator_dev *rdev,
					unsigned int selector);
int regulator_get_voltage_sel_regmap(struct regulator_dev *rdev);
int regulator_map_voltage_iterate(struct regulator_dev *rdev,
				  int min_uV, int max_uV);
int regulator_bulk_get(struct device_d *dev, int num_consumers,
		       struct regulator_bulk_data *consumers);
int regulator_bulk_enable(int num_consumers,
			  struct regulator_bulk_data *consumers);
int regulator_bulk_disable(int num_consumers,
			   struct regulator_bulk_data *consumers);
void regulator_bulk_free(int num_consumers,
			 struct regulator_bulk_data *consumers);

/*
 * Helper functions intended to be used by regulator drivers prior registering
 * their regulators.
 */
int regulator_desc_list_voltage_linear_range(const struct regulator_desc *desc,
					     unsigned int selector);
#else

static inline struct regulator *regulator_get(struct device_d *dev, const char *id)
{
	return NULL;
}

static inline struct regulator *regulator_get_name(const char *name)
{
	return NULL;
}

static inline int regulator_enable(struct regulator *r)
{
	return 0;
}

static inline int regulator_disable(struct regulator *r)
{
	return 0;
}

static inline int regulator_set_voltage(struct regulator *regulator,
					int min_uV, int max_uV)
{
	return 0;
}

static inline int regulator_bulk_get(struct device_d *dev, int num_consumers,
				     struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_enable(int num_consumers,
					struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_disable(int num_consumers,
					 struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline void regulator_bulk_free(int num_consumers,
				       struct regulator_bulk_data *consumers)
{
}

#endif

#endif /* __REGULATOR_H */
