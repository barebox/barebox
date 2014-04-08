#ifndef __REGULATOR_H
#define __REGULATOR_H

/* struct regulator is an opaque object for consumers */
struct regulator;

struct regulator_dev {
	struct regulator_ops *ops;
};

struct regulator_ops {
	/* enable/disable regulator */
	int (*enable) (struct regulator_dev *);
	int (*disable) (struct regulator_dev *);
	int (*is_enabled) (struct regulator_dev *);
};

int of_regulator_register(struct regulator_dev *rd, struct device_node *node);

void regulators_print(void);

#ifdef CONFIG_REGULATOR

struct regulator *regulator_get(struct device_d *, const char *);
int regulator_enable(struct regulator *);
int regulator_disable(struct regulator *);

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

#endif

#endif /* __REGULATOR_H */
