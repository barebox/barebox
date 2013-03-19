#ifndef PARAM_H
#define PARAM_H

#include <linux/types.h>
#include <linux/list.h>

#define PARAM_FLAG_RO	(1 << 0)

struct device_d;
typedef unsigned long          IPaddr_t;

struct param_d {
	const char* (*get)(struct device_d *, struct param_d *param);
	int (*set)(struct device_d *, struct param_d *param, const char *val);
	unsigned int flags;
	char *name;
	char *value;
	struct list_head list;
};

#ifdef CONFIG_PARAMETER
const char *dev_get_param(struct device_d *dev, const char *name);
int dev_set_param(struct device_d *dev, const char *name, const char *val);
struct param_d *get_param_by_name(struct device_d *dev, const char *name);

int dev_add_param(struct device_d *dev, const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags);

int dev_add_param_fixed(struct device_d *dev, char *name, char *value);

void dev_remove_param(struct device_d *dev, char *name);

void dev_remove_parameters(struct device_d *dev);

int dev_param_set_generic(struct device_d *dev, struct param_d *p,
		const char *val);

/* Convenience functions to handle a parameter as an ip address */
int dev_set_param_ip(struct device_d *dev, char *name, IPaddr_t ip);
IPaddr_t dev_get_param_ip(struct device_d *dev, char *name);
#else
static inline const char *dev_get_param(struct device_d *dev, const char *name)
{
	return NULL;
}
static inline int dev_set_param(struct device_d *dev, const char *name,
				const char *val)
{
	return 0;
}
static inline struct param_d *get_param_by_name(struct device_d *dev,
						const char *name)
{
	return NULL;
}

static inline int dev_add_param(struct device_d *dev, char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags)
{
	return 0;
}

static inline int dev_add_param_fixed(struct device_d *dev, char *name, char *value)
{
	return 0;
}

static inline void dev_remove_param(struct device_d *dev, char *name) {}

static inline void dev_remove_parameters(struct device_d *dev) {}

static inline int dev_param_set_generic(struct device_d *dev, struct param_d *p,
		const char *val)
{
	return 0;
}

/* Convenience functions to handle a parameter as an ip address */
static inline int dev_set_param_ip(struct device_d *dev, char *name, IPaddr_t ip)
{
	return 0;
}
static inline IPaddr_t dev_get_param_ip(struct device_d *dev, char *name)
{
	return 0;
}
#endif

#endif /* PARAM_H */

