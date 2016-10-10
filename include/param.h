#ifndef PARAM_H
#define PARAM_H

#include <linux/err.h>
#include <linux/types.h>
#include <linux/list.h>

#define PARAM_FLAG_RO	(1 << 0)
#define PARAM_GLOBALVAR_UNQUALIFIED	(1 << 1)

struct device_d;
typedef uint32_t          IPaddr_t;

struct param_d {
	const char* (*get)(struct device_d *, struct param_d *param);
	int (*set)(struct device_d *, struct param_d *param, const char *val);
	void (*info)(struct param_d *param);
	unsigned int flags;
	char *name;
	char *value;
	struct device_d *dev;
	void *driver_priv;
	struct list_head list;
};

#ifdef CONFIG_PARAMETER
const char *dev_get_param(struct device_d *dev, const char *name);
int dev_set_param(struct device_d *dev, const char *name, const char *val);
struct param_d *get_param_by_name(struct device_d *dev, const char *name);

struct param_d *dev_add_param(struct device_d *dev, const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags);

struct param_d *dev_add_param_string(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		char **value, void *priv);

struct param_d *dev_add_param_int(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char *format, void *priv);

struct param_d *dev_add_param_bool(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, void *priv);

struct param_d *dev_add_param_enum(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char * const *names, int max, void *priv);

struct param_d *dev_add_param_bitmask(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		unsigned long *value, const char * const *names, int max, void *priv);

struct param_d *dev_add_param_int_ro(struct device_d *dev, const char *name,
		int value, const char *format);

struct param_d *dev_add_param_llint_ro(struct device_d *dev, const char *name,
		long long value, const char *format);

struct param_d *dev_add_param_ip(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		IPaddr_t *ip, void *priv);

struct param_d *dev_add_param_mac(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		u8 *mac, void *priv);

int dev_add_param_fixed(struct device_d *dev, const char *name, const char *value);

void dev_remove_param(struct param_d *p);

void dev_remove_parameters(struct device_d *dev);

int dev_param_set_generic(struct device_d *dev, struct param_d *p,
		const char *val);

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

static inline struct param_d *dev_add_param(struct device_d *dev, char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *, struct param_d *p),
		unsigned long flags)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_string(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		char **value, void *priv)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_int(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char *format, void *priv)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_enum(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char * const *names, int max, void *priv)

{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_bool(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, void *priv)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_int_ro(struct device_d *dev, const char *name,
		int value, const char *format)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_llint_ro(struct device_d *dev, const char *name,
		long long value, const char *format)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_ip(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		IPaddr_t *ip, void *priv)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct param_d *dev_add_param_mac(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		u8 *mac, void *priv)
{
	return ERR_PTR(-ENOSYS);
}

static inline int dev_add_param_fixed(struct device_d *dev, const char *name, const char *value)
{
	return 0;
}

static inline void dev_remove_param(struct param_d *p) {}

static inline void dev_remove_parameters(struct device_d *dev) {}

static inline int dev_param_set_generic(struct device_d *dev, struct param_d *p,
		const char *val)
{
	return 0;
}
#endif

#endif /* PARAM_H */
