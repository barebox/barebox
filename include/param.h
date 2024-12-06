/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PARAM_H
#define PARAM_H

#include <linux/err.h>
#include <linux/types.h>
#include <linux/list.h>

#define PARAM_FLAG_RO	(1 << 0)
#define PARAM_GLOBALVAR_UNQUALIFIED	(1 << 1)

struct device;
struct file_list;
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
	PARAM_TYPE_FILE_LIST,
};

struct param_d {
	const char* (*get)(struct device *, struct param_d *param);
	int (*set)(struct device *, struct param_d *param, const char *val);
	void (*info)(struct param_d *param);
	unsigned int flags;
	const char *name;
	char *value;
	struct device *dev;
	void *driver_priv;
	struct list_head list;
	enum param_type type;
};

enum param_tristate { PARAM_TRISTATE_UNKNOWN, PARAM_TRISTATE_TRUE, PARAM_TRISTATE_FALSE };

#ifdef CONFIG_PARAMETER
const char *get_param_type(struct param_d *param);
const char *dev_get_param(struct device *dev, const char *name);
int dev_set_param(struct device *dev, const char *name, const char *val);
struct param_d *get_param_by_name(struct device *dev, const char *name);

struct param_d *dev_add_param(struct device *dev, const char *name,
			      int (*set)(struct device *dev, struct param_d *p, const char *val),
			      const char *(*get)(struct device *, struct param_d *p),
			      unsigned long flags);

struct param_d *dev_add_param_string(struct device *dev, const char *name,
				     int (*set)(struct param_d *p, void *priv),
				     int (*get)(struct param_d *p, void *priv),
				     char **value, void *priv);

struct param_d *__dev_add_param_int(struct device *dev, const char *name,
				    int (*set)(struct param_d *p, void *priv),
				    int (*get)(struct param_d *p, void *priv),
				    void *value, enum param_type type,
				    const char *format, void *priv);

struct param_d *dev_add_param_enum(struct device *dev, const char *name,
				   int (*set)(struct param_d *p, void *priv),
				   int (*get)(struct param_d *p, void *priv),
				   int *value, const char * const *names,
				   int num_names, void *priv);

struct param_d *dev_add_param_tristate(struct device *dev, const char *name,
				       int (*set)(struct param_d *p, void *priv),
				       int (*get)(struct param_d *p, void *priv),
				       int *value, void *priv);

struct param_d *dev_add_param_tristate_ro(struct device *dev,
					  const char *name,
					  int *value);

struct param_d *dev_add_param_bitmask(struct device *dev, const char *name,
				      int (*set)(struct param_d *p, void *priv),
				      int (*get)(struct param_d *p, void *priv),
				      unsigned long *value,
				      const char * const *names, int num_names,
				      void *priv);

struct param_d *dev_add_param_ip(struct device *dev, const char *name,
				 int (*set)(struct param_d *p, void *priv),
				 int (*get)(struct param_d *p, void *priv),
				 IPaddr_t *ip, void *priv);

struct param_d *dev_add_param_mac(struct device *dev, const char *name,
				  int (*set)(struct param_d *p, void *priv),
				  int (*get)(struct param_d *p, void *priv),
				  u8 *mac, void *priv);

struct param_d *dev_add_param_file_list(struct device *dev, const char *name,
					int (*set)(struct param_d *p, void *priv),
					int (*get)(struct param_d *p, void *priv),
					struct file_list **file_list,
					void *priv);

struct param_d *dev_add_param_fixed(struct device *dev, const char *name,
				    const char *value);

void dev_remove_param(struct param_d *p);

void dev_remove_parameters(struct device *dev);

int dev_param_set_generic(struct device *dev, struct param_d *p,
			  const char *val);

#else
static inline const char *dev_get_param(struct device *dev, const char *name)
{
	return NULL;
}
static inline int dev_set_param(struct device *dev, const char *name,
				const char *val)
{
	return 0;
}
static inline struct param_d *get_param_by_name(struct device *dev,
						const char *name)
{
	return NULL;
}

static inline struct param_d *dev_add_param(struct device *dev,
					    const char *name,
					    int (*set)(struct device *dev, struct param_d *p, const char *val),
					    const char *(*get)(struct device *, struct param_d *p),
					    unsigned long flags)
{
	return NULL;
}

static inline struct param_d *dev_add_param_string(struct device *dev,
						   const char *name,
						   int (*set)(struct param_d *p, void *priv),
						   int (*get)(struct param_d *p, void *priv),
						   char **value, void *priv)
{
	return NULL;
}

static inline struct param_d *__dev_add_param_int(struct device *dev,
						  const char *name,
						  int (*set)(struct param_d *p, void *priv),
						  int (*get)(struct param_d *p, void *priv),
						  void *value,
						  enum param_type type,
						  const char *format,
						  void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_enum(struct device *dev,
						 const char *name,
						 int (*set)(struct param_d *p, void *priv),
						 int (*get)(struct param_d *p, void *priv),
						 int *value,
						 const char * const *names,
						 int num_names, void *priv)

{
	return NULL;
}

static inline struct param_d *dev_add_param_bitmask(struct device *dev,
						    const char *name,
						    int (*set)(struct param_d *p, void *priv),
						    int (*get)(struct param_d *p, void *priv),
						    unsigned long *value,
						    const char * const *names,
						    int num_names, void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_tristate(struct device *dev,
						     const char *name,
						     int (*set)(struct param_d *p, void *priv),
						     int (*get)(struct param_d *p, void *priv),
						     int *value, void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_tristate_ro(struct device *dev,
							const char *name,
							int *value)
{
	return NULL;
}

static inline struct param_d *dev_add_param_ip(struct device *dev,
					       const char *name,
					       int (*set)(struct param_d *p, void *priv),
					       int (*get)(struct param_d *p, void *priv),
					       IPaddr_t *ip, void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_mac(struct device *dev,
						const char *name,
						int (*set)(struct param_d *p, void *priv),
						int (*get)(struct param_d *p, void *priv),
						u8 *mac, void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_file_list(struct device *dev,
						      const char *name,
						      int (*set)(struct param_d *p, void *priv),
						      int (*get)(struct param_d *p, void *priv),
						      struct file_list **file_list,
						      void *priv)
{
	return NULL;
}

static inline struct param_d *dev_add_param_fixed(struct device *dev,
						  const char *name,
						  const char *value)
{
	return NULL;
}

static inline void dev_remove_param(struct param_d *p) {}

static inline void dev_remove_parameters(struct device *dev) {}

static inline int dev_param_set_generic(struct device *dev, struct param_d *p,
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
	static inline struct param_d *dev_add_param_##intname(struct device *dev, const char *name,	\
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
	static inline struct param_d *dev_add_param_##intname##_fixed(struct device *dev, const char *name,	\
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
	static inline struct param_d *dev_add_param_##intname##_ro(struct device *dev, const char *name,		\
			inttype *value, const char *format)								\
	{														\
		return __dev_add_param_int(dev, name, param_set_readonly, NULL, value, paramtype, format, NULL);	\
	}

DECLARE_PARAM_INT_RO(int, int, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_RO(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_RO(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_PARAM_INT_RO(int64, int64_t, PARAM_TYPE_INT64)
DECLARE_PARAM_INT_RO(uint64, uint64_t, PARAM_TYPE_UINT64)

static inline struct param_d *dev_add_param_bool(struct device *dev,
						 const char *name,
						 int (*set)(struct param_d *p, void *priv),
						 int (*get)(struct param_d *p, void *priv),
						 uint32_t *value, void *priv)
{
	return __dev_add_param_int(dev, name, set, get, value, PARAM_TYPE_BOOL, "%u", priv);
}

static inline struct param_d *dev_add_param_bool_fixed(struct device *dev,
						       const char *name,
						       uint32_t value)
{
	return __dev_add_param_int(dev, name, ERR_PTR(-EROFS), NULL, &value, PARAM_TYPE_BOOL,
				   "%u", NULL);
}

static inline struct param_d *dev_add_param_bool_ro(struct device *dev,
						    const char *name,
						    uint32_t *value)
{
	return __dev_add_param_int(dev, name, param_set_readonly, NULL, value, PARAM_TYPE_BOOL,
				   "%u", NULL);
}

static inline struct param_d *dev_add_param_string_ro(struct device *dev,
						      const char *name,
						      char **value)
{
	return dev_add_param_string(dev, name, param_set_readonly, NULL, value, NULL);
}

static inline struct param_d *dev_add_param_string_fixed(struct device *dev,
							 const char *name,
							 const char *value)
{
	return dev_add_param_fixed(dev, name, value);
}

static inline struct param_d *dev_add_param_enum_ro(struct device *dev,
						    const char *name,
						    int *value,
						    const char * const *names,
						    int num_names)
{
	return dev_add_param_enum(dev, name, param_set_readonly, NULL,
				  value, names, num_names, NULL);
}

static inline struct param_d *dev_add_param_bitmask_ro(struct device *dev,
						       const char *name,
						       unsigned long *value,
						       const char * const *names,
						       int num_names)
{
	return dev_add_param_bitmask(dev, name, param_set_readonly, NULL,
				     value, names, num_names, NULL);
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
