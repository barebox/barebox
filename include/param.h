#ifndef PARAM_H
#define PARAM_H

#include <linux/err.h>
#include <linux/types.h>
#include <linux/list.h>

#define PARAM_FLAG_RO	(1 << 0)
#define PARAM_GLOBALVAR_UNQUALIFIED	(1 << 1)

struct device_d;
typedef uint32_t          IPaddr_t;

enum param_type {
	PARAM_TYPE_STRING = 0,
	PARAM_TYPE_INT32,
	PARAM_TYPE_UINT32,
	PARAM_TYPE_INT64,
	PARAM_TYPE_UINT64,
	PARAM_TYPE_BOOL,
	PARAM_TYPE_ENUM,
	PARAM_TYPE_BITMASK,
	PARAM_TYPE_IPV4,
	PARAM_TYPE_MAC,
};

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
	enum param_type type;
};

#ifdef CONFIG_PARAMETER
const char *get_param_type(struct param_d *param);
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

struct param_d *__dev_add_param_int(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		void *value, enum param_type type, const char *format, void *priv);

struct param_d *dev_add_param_enum(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char * const *names, int max, void *priv);

struct param_d *dev_add_param_bitmask(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		unsigned long *value, const char * const *names, int max, void *priv);

struct param_d *dev_add_param_ip(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		IPaddr_t *ip, void *priv);

struct param_d *dev_add_param_mac(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		u8 *mac, void *priv);

struct param_d *dev_add_param_fixed(struct device_d *dev, const char *name, const char *value);

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

static inline struct param_d *dev_add_param(struct device_d *dev, const char *name,
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

static inline struct param_d *__dev_add_param_int(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		void *value, enum param_type type, const char *format, void *priv)
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

static inline struct param_d *dev_add_param_bitmask(struct device_d *dev, const char *name,
                int (*set)(struct param_d *p, void *priv),
                int (*get)(struct param_d *p, void *priv),
                unsigned long *value, const char * const *names, int max, void *priv)
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

static inline struct param_d *dev_add_param_fixed(struct device_d *dev, const char *name,
						  const char *value)
{
	return ERR_PTR(-ENOSYS);
}

static inline void dev_remove_param(struct param_d *p) {}

static inline void dev_remove_parameters(struct device_d *dev) {}

static inline int dev_param_set_generic(struct device_d *dev, struct param_d *p,
		const char *val)
{
	return 0;
}
#endif

int param_set_readonly(struct param_d *p, void *priv);

/*
 * dev_add_param_int
 * dev_add_param_int32
 * dev_add_param_uint32
 * dev_add_param_int64
 * dev_add_param_uint64
 */
#define DECLARE_PARAM_INT(intname, inttype, paramtype) \
	static inline struct param_d *dev_add_param_##intname(struct device_d *dev, const char *name,	\
			int (*set)(struct param_d *p, void *priv),					\
			int (*get)(struct param_d *p, void *priv),					\
			inttype *value, const char *format, void *priv)					\
	{												\
		return __dev_add_param_int(dev, name, set, get, value, paramtype, format, priv);	\
	}

DECLARE_PARAM_INT(int, int, PARAM_TYPE_INT32)
DECLARE_PARAM_INT(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_PARAM_INT(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_PARAM_INT(int64, int64_t, PARAM_TYPE_INT64)
DECLARE_PARAM_INT(uint64, uint64_t, PARAM_TYPE_UINT64)

/*
 * dev_add_param_int_fixed
 * dev_add_param_int32_fixed
 * dev_add_param_uint32_fixed
 * dev_add_param_int64_fixed
 * dev_add_param_uint64_fixed
 */
#define DECLARE_PARAM_INT_FIXED(intname, inttype, paramtype) \
	static inline struct param_d *dev_add_param_##intname##_fixed(struct device_d *dev, const char *name,	\
			inttype value, const char *format)							\
	{													\
		return __dev_add_param_int(dev, name, ERR_PTR(-EROFS), NULL, &value, paramtype, format, NULL);	\
	}

DECLARE_PARAM_INT_FIXED(int, int, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_FIXED(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_FIXED(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_PARAM_INT_FIXED(int64, int64_t, PARAM_TYPE_INT64)
DECLARE_PARAM_INT_FIXED(uint64, uint64_t, PARAM_TYPE_UINT64)

/*
 * dev_add_param_int_ro
 * dev_add_param_int32_ro
 * dev_add_param_uint32_ro
 * dev_add_param_int64_ro
 * dev_add_param_uint64_ro
 */
#define DECLARE_PARAM_INT_RO(intname, inttype, paramtype) \
	static inline struct param_d *dev_add_param_##intname##_ro(struct device_d *dev, const char *name,		\
			inttype *value, const char *format)								\
	{														\
		return __dev_add_param_int(dev, name, param_set_readonly, NULL, value, paramtype, format, NULL);	\
	}

DECLARE_PARAM_INT_RO(int, int, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_RO(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_RO(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_PARAM_INT_RO(int64, int64_t, PARAM_TYPE_INT64)
DECLARE_PARAM_INT_RO(uint64, uint64_t, PARAM_TYPE_UINT64)

static inline struct param_d *dev_add_param_bool(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		uint32_t *value, void *priv)
{
	return __dev_add_param_int(dev, name, set, get, value, PARAM_TYPE_BOOL, "%u", priv);
}

static inline struct param_d *dev_add_param_bool_fixed(struct device_d *dev, const char *name,
		uint32_t value)
{
	return __dev_add_param_int(dev, name, ERR_PTR(-EROFS), NULL, &value, PARAM_TYPE_BOOL,
				   "%u", NULL);
}

static inline struct param_d *dev_add_param_bool_ro(struct device_d *dev, const char *name,
		uint32_t *value)
{
	return __dev_add_param_int(dev, name, param_set_readonly, NULL, value, PARAM_TYPE_BOOL,
				   "%u", NULL);
}

static inline struct param_d *dev_add_param_string_ro(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		char **value, void *priv)
{
	return dev_add_param_string(dev, name, param_set_readonly, NULL, value, NULL);
}

static inline struct param_d *dev_add_param_string_fixed(struct device_d *dev, const char *name,
		char *value)
{
	return dev_add_param_fixed(dev, name, value);
}

static inline struct param_d *dev_add_param_enum_ro(struct device_d *dev, const char *name,
		int *value, const char * const *names, int max)
{
	return dev_add_param_enum(dev, name, param_set_readonly, NULL,
				  value, names, max, NULL);
}

static inline struct param_d *dev_add_param_bitmask_ro(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		unsigned long *value, const char * const *names, int max, void *priv)
{
	return dev_add_param_bitmask(dev, name, param_set_readonly, NULL,
				     value, names, max, NULL);
}

/*
 * unimplemented:
 * dev_add_param_enum_fixed
 * dev_add_param_bitmask_fixed
 * dev_add_param_ip_ro
 * dev_add_param_ip_fixed
 * dev_add_param_mac_ro
 * dev_add_param_mac_fixed
 */
#endif /* PARAM_H */
