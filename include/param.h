#ifndef PARAM_H
#define PARAM_H

#include <net.h>

#define PARAM_TYPE_STRING	1
#define PARAM_TYPE_ULONG	2
#define PARAM_TYPE_IPADDR	3

#define PARAM_FLAG_RO	(1 << 0)

struct device_d;

typedef union {
	char *val_str;
	ulong val_ulong;
	IPaddr_t val_ip;
} value_t;

struct param_d {
        struct param_d* (*get)(struct device_d *, struct param_d *param);
        int (*set)(struct device_d *, struct param_d *param, value_t val);
	ulong type;
	ulong flags;
        char *name;
        ulong cookie;
        struct param_d *next;
	value_t value;
};

struct param_d* dev_get_param(struct device_d *dev, char *name);
int dev_set_param(struct device_d *dev, char *name, value_t val);
struct param_d *get_param_by_name(struct device_d *dev, char *name);
void print_param(struct param_d *param);
IPaddr_t dev_get_param_ip(struct device_d *dev, char *name);
int dev_set_param_ip(struct device_d *dev, char *name, IPaddr_t ip);

int dev_add_parameter(struct device_d *dev, struct param_d *par);

int global_add_parameter(struct param_d *param);

#endif /* PARAM_H */

