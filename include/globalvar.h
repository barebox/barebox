#ifndef __GLOBALVAR_H
#define __GLOBALVAR_H

int globalvar_add_simple(const char *name);

int globalvar_add(const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags);
char *globalvar_get_match(const char *match, const char *seperator);

#endif /* __GLOBALVAR_H */
