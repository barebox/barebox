#ifndef __REGULATOR_H
#define __REGULATOR_H

/* struct regulator is an opaque object for consumers */
struct regulator;

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
};

struct regulator_dev {
	const struct regulator_desc *desc;
	struct regmap *regmap;
	int boot_on;
};

struct regulator_ops {
	/* enable/disable regulator */
	int (*enable) (struct regulator_dev *);
	int (*disable) (struct regulator_dev *);
	int (*is_enabled) (struct regulator_dev *);

	int (*list_voltage) (struct regulator_dev *, unsigned int);
	int (*set_voltage_sel) (struct regulator_dev *, unsigned int);
};

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
int regulator_enable(struct regulator *);
int regulator_disable(struct regulator *);
int regulator_is_enabled_regmap(struct regulator_dev *);
int regulator_enable_regmap(struct regulator_dev *);
int regulator_disable_regmap(struct regulator_dev *);
int regulator_set_voltage_sel_regmap(struct regulator_dev *, unsigned);
int regulator_set_voltage(struct regulator *regulator, int min_uV, int max_uV);
int regulator_map_voltage_linear(struct regulator_dev *rdev,
				 int min_uV, int max_uV);
int regulator_list_voltage_linear(struct regulator_dev *rdev,
				  unsigned int selector);
#else

static inline struct regulator *regulator_get(struct device_d *dev, const char *id)
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

#endif

#endif /* __REGULATOR_H */
