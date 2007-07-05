#ifndef PARAM_H
#define PARAM_H

#include <linux/types.h>

#define PARAM_FLAG_RO	(1 << 0)

struct device_d;
typedef unsigned long          IPaddr_t;

struct param_d {
	char* (*get)(struct device_d *, struct param_d *param);
	int (*set)(struct device_d *, struct param_d *param, const char *val);
	unsigned int flags;
	char *name;
	struct param_d *next;
	char *value;
};

const char *dev_get_param(struct device_d *dev, const char *name);
int dev_set_param(struct device_d *dev, const char *name, const char *val);
struct param_d *get_param_by_name(struct device_d *dev, const char *name);
int dev_add_param(struct device_d *dev, struct param_d *par);

/* Convenience functions to handle a parameter as an ip address */
int dev_set_param_ip(struct device_d *dev, char *name, IPaddr_t ip);
IPaddr_t dev_get_param_ip(struct device_d *dev, char *name);

int global_add_parameter(struct param_d *param);

#endif /* PARAM_H */

