#ifndef __GLOBALVAR_H
#define __GLOBALVAR_H

#include <param.h>
#include <driver.h>
#include <linux/err.h>

extern struct device_d global_device;

#ifdef CONFIG_GLOBALVAR
int globalvar_add_simple(const char *name, const char *value);

int globalvar_add(const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags);
char *globalvar_get_match(const char *match, const char *separator);
void globalvar_set_match(const char *match, const char *val);

static inline int globalvar_add_simple_int(const char *name,
		int *value, const char *format)
{
	struct param_d *p;

	p = dev_add_param_int(&global_device, name, NULL, NULL,
		value, format, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
}

static inline int globalvar_add_simple_bool(const char *name,
		int *value)
{
	struct param_d *p;

	p = dev_add_param_bool(&global_device, name, NULL, NULL,
		value, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
}

static inline int globalvar_add_simple_enum(const char *name,
		int *value, const char **names, int max)
{
	struct param_d *p;

	p = dev_add_param_enum(&global_device, name, NULL, NULL,
		value, names, max, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
}

static inline int globalvar_add_simple_ip(const char *name,
		IPaddr_t *ip)
{
	struct param_d *p;

	p = dev_add_param_ip(&global_device, name, NULL, NULL,
		ip, NULL);

	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
}
#else
static inline int globalvar_add_simple(const char *name, const char *value)
{
	return 0;
}

static inline int globalvar_add_simple_int(const char *name,
		int *value, const char *format)
{
	return 0;
}

static inline int globalvar_add_simple_bool(const char *name,
		int *value)
{
	return 0;
}

static inline int globalvar_add_simple_enum(const char *name,
		int *value, const char **names, int max)
{
	return 0;
}

static inline int globalvar_add_simple_ip(const char *name,
		IPaddr_t *ip)
{
	return 0;
}

static inline int globalvar_add(const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags)
{
	return 0;
}

static inline char *globalvar_get_match(const char *match, const char *separator)
{
	return NULL;
}

static inline void globalvar_set_match(const char *match, const char *val) {}
#endif

#endif /* __GLOBALVAR_H */
