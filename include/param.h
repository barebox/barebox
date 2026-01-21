/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef PARAM_H
#define PARAM_H

#include <linux/err.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/uuid.h>
#include <bobject.h>
#include <stdarg.h>

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
	PARAM_TYPE_UUID,
	PARAM_TYPE_GUID,
};

struct param_d {
	const char* (*get)(struct bobject *, struct param_d *param);
	int (*set)(struct bobject *, struct param_d *param, const char *val);
	void (*info)(struct param_d *param);
	unsigned int flags;
	const char *name;
	char *value;
	struct bobject *bobj;
	void *driver_priv;
	struct list_head list;
	enum param_type type;
};

#define PARAM_ENUM_UNKNOWN	(-1000000)

enum param_tristate { PARAM_TRISTATE_UNKNOWN, PARAM_TRISTATE_TRUE, PARAM_TRISTATE_FALSE };

#ifdef CONFIG_PARAMETER
const char *get_param_type(struct param_d *param);
const char *bobject_get_param(bobject_t bobj, const char *name);
int bobject_set_param(bobject_t bobj, const char *name, const char *val);
struct param_d *get_param_by_name(bobject_t bobj, const char *name);

struct param_d *bobject_add_param(bobject_t bobj, const char *name,
			      int (*set)(bobject_t bobj, struct param_d *p, const char *val),
			      const char *(*get)(bobject_t, struct param_d *p),
			      unsigned long flags);

struct param_d *bobject_add_param_string(bobject_t bobj, const char *name,
				     int (*set)(struct param_d *p, void *priv),
				     int (*get)(struct param_d *p, void *priv),
				     char **value, void *priv);

struct param_d *__bobject_add_param_int(bobject_t bobj, const char *name,
				    int (*set)(struct param_d *p, void *priv),
				    int (*get)(struct param_d *p, void *priv),
				    void *value, enum param_type type,
				    const char *format, void *priv);

struct param_d *bobject_add_param_enum(bobject_t bobj, const char *name,
				   int (*set)(struct param_d *p, void *priv),
				   int (*get)(struct param_d *p, void *priv),
				   int *value, const char * const *names,
				   int num_names, void *priv);

struct param_d *bobject_add_param_tristate(bobject_t bobj, const char *name,
				       int (*set)(struct param_d *p, void *priv),
				       int (*get)(struct param_d *p, void *priv),
				       int *value, void *priv);

struct param_d *bobject_add_param_tristate_ro(bobject_t bobj,
					  const char *name,
					  int *value);

struct param_d *bobject_add_param_bitmask(bobject_t bobj, const char *name,
				      int (*set)(struct param_d *p, void *priv),
				      int (*get)(struct param_d *p, void *priv),
				      unsigned long *value,
				      const char * const *names, int num_names,
				      void *priv);

struct param_d *bobject_add_param_ip(bobject_t bobj, const char *name,
				 int (*set)(struct param_d *p, void *priv),
				 int (*get)(struct param_d *p, void *priv),
				 IPaddr_t *ip, void *priv);

struct param_d *bobject_add_param_mac(bobject_t bobj, const char *name,
				  int (*set)(struct param_d *p, void *priv),
				  int (*get)(struct param_d *p, void *priv),
				  u8 *mac, void *priv);

struct param_d *bobject_add_param_file_list(bobject_t bobj, const char *name,
					int (*set)(struct param_d *p, void *priv),
					int (*get)(struct param_d *p, void *priv),
					struct file_list **file_list,
					void *priv);

struct param_d *__bobject_add_param_uuid(bobject_t _bobj, const char *name,
					 int (*set)(struct param_d *p, void *priv),
					 int (*get)(struct param_d *p, void *priv),
					 void *uuid, bool is_guid,
					 void *priv);

struct param_d *vbobject_add_param_fixed(struct bobject *bobj, const char *name,
					 const char *fmt, va_list ap);

struct param_d *bobject_add_param_fixed(bobject_t bobj, const char *name,
				    const char *fmt, ...)
	__printf(3, 4);

void param_remove(struct param_d *p);

int bobject_param_set_generic(bobject_t bobj, struct param_d *p,
			  const char *val);

int param_int_set_scale(struct param_d *p, uint64_t max);

#else
static inline const char *bobject_get_param(bobject_t bobj, const char *name)
{
	return NULL;
}
static inline int bobject_set_param(bobject_t bobj, const char *name,
				const char *val)
{
	return 0;
}
static inline struct param_d *get_param_by_name(bobject_t bobj,
						const char *name)
{
	return NULL;
}

static inline struct param_d *bobject_add_param(bobject_t bobj,
					    const char *name,
					    int (*set)(bobject_t bobj, struct param_d *p, const char *val),
					    const char *(*get)(bobject_t, struct param_d *p),
					    unsigned long flags)
{
	return NULL;
}

static inline struct param_d *bobject_add_param_string(bobject_t bobj,
						   const char *name,
						   int (*set)(struct param_d *p, void *priv),
						   int (*get)(struct param_d *p, void *priv),
						   char **value, void *priv)
{
	return NULL;
}

static inline struct param_d *__bobject_add_param_int(bobject_t bobj,
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

static inline struct param_d *bobject_add_param_enum(bobject_t bobj,
						 const char *name,
						 int (*set)(struct param_d *p, void *priv),
						 int (*get)(struct param_d *p, void *priv),
						 int *value,
						 const char * const *names,
						 int num_names, void *priv)

{
	return NULL;
}

static inline struct param_d *bobject_add_param_bitmask(bobject_t bobj,
						    const char *name,
						    int (*set)(struct param_d *p, void *priv),
						    int (*get)(struct param_d *p, void *priv),
						    unsigned long *value,
						    const char * const *names,
						    int num_names, void *priv)
{
	return NULL;
}

static inline struct param_d *bobject_add_param_tristate(bobject_t bobj,
						     const char *name,
						     int (*set)(struct param_d *p, void *priv),
						     int (*get)(struct param_d *p, void *priv),
						     int *value, void *priv)
{
	return NULL;
}

static inline struct param_d *bobject_add_param_tristate_ro(bobject_t bobj,
							const char *name,
							int *value)
{
	return NULL;
}

static inline struct param_d *bobject_add_param_ip(bobject_t bobj,
					       const char *name,
					       int (*set)(struct param_d *p, void *priv),
					       int (*get)(struct param_d *p, void *priv),
					       IPaddr_t *ip, void *priv)
{
	return NULL;
}

static inline struct param_d *bobject_add_param_mac(bobject_t bobj,
						const char *name,
						int (*set)(struct param_d *p, void *priv),
						int (*get)(struct param_d *p, void *priv),
						u8 *mac, void *priv)
{
	return NULL;
}

static inline struct param_d *bobject_add_param_file_list(bobject_t bobj,
						      const char *name,
						      int (*set)(struct param_d *p, void *priv),
						      int (*get)(struct param_d *p, void *priv),
						      struct file_list **file_list,
						      void *priv)
{
	return NULL;
}

static inline struct param_d *__bobject_add_param_uuid(bobject_t _bobj, const char *name,
						       int (*set)(struct param_d *p, void *priv),
						       int (*get)(struct param_d *p, void *priv),
						       void *uuid, bool is_guid,
						       void *priv)
{
	return NULL;
}

static inline
struct param_d *vbobject_add_param_fixed(struct bobject *bobj, const char *name,
					 const char *fmt, va_list ap)
{
	return NULL;
}

static inline __printf(3, 4)
struct param_d *bobject_add_param_fixed(bobject_t bobj,
					const char *name,
					const char *fmt, ...)
{
	return NULL;
}

static inline void param_remove(struct param_d *p) {}

static inline int bobject_param_set_generic(bobject_t bobj, struct param_d *p,
					const char *val)
{
	return 0;
}

static inline int param_int_set_scale(struct param_d *p, uint64_t max)
{
	return 0;
}

#endif

static inline const char *get_param_value(struct param_d *param)
{
	if (!IS_ENABLED(CONFIG_PARAMETER))
		return NULL;
	if (IS_ERR_OR_NULL(param))
		return ERR_CAST(param);

	return param->get(param->bobj, param);
}

static inline struct param_d *
bobject_add_param_uuid(bobject_t _bobj, const char *name,
		       int (*set)(struct param_d *p, void *priv),
		       int (*get)(struct param_d *p, void *priv),
		       uuid_t *uuid, void *priv)
{
	return __bobject_add_param_uuid(_bobj, name, set, get, uuid, false, priv);
}

static inline struct param_d *
bobject_add_param_guid(bobject_t _bobj, const char *name,
		       int (*set)(struct param_d *p, void *priv),
		       int (*get)(struct param_d *p, void *priv),
		       guid_t *guid, void *priv)
{
	return __bobject_add_param_uuid(_bobj, name, set, get, guid, true, priv);
}


int param_set_readonly(struct param_d *p, void *priv);

/*
 * bobject_add_param_int
 * bobject_add_param_int32
 * bobject_add_param_uint32
 * bobject_add_param_int64
 * bobject_add_param_uint64
 */
#define DECLARE_PARAM_INT(intname, inttype, paramtype) \
	static inline struct param_d *bobject_add_param_##intname(bobject_t bobj, const char *name,	\
			int (*set)(struct param_d *p, void *priv),					\
			int (*get)(struct param_d *p, void *priv),					\
			inttype *value, const char *format, void *priv)					\
	{												\
		return __bobject_add_param_int(bobj, name, set, get, value, paramtype, format, priv);	\
	}

DECLARE_PARAM_INT(int, int, PARAM_TYPE_INT32)
DECLARE_PARAM_INT(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_PARAM_INT(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_PARAM_INT(int64, int64_t, PARAM_TYPE_INT64)
DECLARE_PARAM_INT(uint64, uint64_t, PARAM_TYPE_UINT64)

/*
 * bobject_add_param_int_fixed
 * bobject_add_param_int32_fixed
 * bobject_add_param_uint32_fixed
 * bobject_add_param_int64_fixed
 * bobject_add_param_uint64_fixed
 */
#define DECLARE_PARAM_INT_FIXED(intname, inttype, paramtype) \
	static inline struct param_d *bobject_add_param_##intname##_fixed(bobject_t bobj, const char *name,	\
			inttype value, const char *format)							\
	{													\
		return __bobject_add_param_int(bobj, name, ERR_PTR(-EROFS), NULL, &value, paramtype, format, NULL);	\
	}

DECLARE_PARAM_INT_FIXED(int, int, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_FIXED(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_FIXED(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_PARAM_INT_FIXED(int64, int64_t, PARAM_TYPE_INT64)
DECLARE_PARAM_INT_FIXED(uint64, uint64_t, PARAM_TYPE_UINT64)

/*
 * bobject_add_param_int_ro
 * bobject_add_param_int32_ro
 * bobject_add_param_uint32_ro
 * bobject_add_param_int64_ro
 * bobject_add_param_uint64_ro
 */
#define DECLARE_PARAM_INT_RO(intname, inttype, paramtype) \
	static inline struct param_d *bobject_add_param_##intname##_ro(bobject_t bobj, const char *name,		\
			inttype *value, const char *format)								\
	{														\
		return __bobject_add_param_int(bobj, name, param_set_readonly, NULL, value, paramtype, format, NULL);	\
	}

DECLARE_PARAM_INT_RO(int, int, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_RO(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_PARAM_INT_RO(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_PARAM_INT_RO(int64, int64_t, PARAM_TYPE_INT64)
DECLARE_PARAM_INT_RO(uint64, uint64_t, PARAM_TYPE_UINT64)

static inline struct param_d *bobject_add_param_bool(bobject_t bobj,
						 const char *name,
						 int (*set)(struct param_d *p, void *priv),
						 int (*get)(struct param_d *p, void *priv),
						 uint32_t *value, void *priv)
{
	return __bobject_add_param_int(bobj, name, set, get, value, PARAM_TYPE_BOOL, "%u", priv);
}

static inline struct param_d *bobject_add_param_bool_fixed(bobject_t bobj,
						       const char *name,
						       uint32_t value)
{
	return __bobject_add_param_int(bobj, name, ERR_PTR(-EROFS), NULL, &value, PARAM_TYPE_BOOL,
				   "%u", NULL);
}

static inline struct param_d *bobject_add_param_bool_ro(bobject_t bobj,
						    const char *name,
						    uint32_t *value)
{
	return __bobject_add_param_int(bobj, name, param_set_readonly, NULL, value, PARAM_TYPE_BOOL,
				   "%u", NULL);
}

static inline struct param_d *bobject_add_param_string_ro(bobject_t bobj,
						      const char *name,
						      char **value)
{
	return bobject_add_param_string(bobj, name, param_set_readonly, NULL, value, NULL);
}

static inline struct param_d *bobject_add_param_string_fixed(bobject_t bobj,
							 const char *name,
							 const char *value)
{
	return bobject_add_param_fixed(bobj, name, "%s", value);
}

static inline struct param_d *bobject_add_param_enum_ro(bobject_t bobj,
						    const char *name,
						    int *value,
						    const char * const *names,
						    int num_names)
{
	return bobject_add_param_enum(bobj, name, param_set_readonly, NULL,
				  value, names, num_names, NULL);
}

static inline struct param_d *bobject_add_param_bitmask_ro(bobject_t bobj,
						       const char *name,
						       unsigned long *value,
						       const char * const *names,
						       int num_names)
{
	return bobject_add_param_bitmask(bobj, name, param_set_readonly, NULL,
				     value, names, num_names, NULL);
}

#define dev_get_param			bobject_get_param
#define dev_set_param			bobject_set_param
#define dev_add_param			bobject_add_param
#define dev_add_param_string		bobject_add_param_string
#define dev_add_param_fixed		bobject_add_param_fixed
#define __dev_add_param_int		__bobject_add_param_int
#define dev_add_param_enum		bobject_add_param_enum
#define dev_add_param_tristate		bobject_add_param_tristate
#define dev_add_param_tristate_ro	bobject_add_param_tristate_ro
#define dev_add_param_bitmask		bobject_add_param_bitmask
#define dev_add_param_ip		bobject_add_param_ip
#define dev_add_param_mac		bobject_add_param_mac
#define dev_add_param_file_list		bobject_add_param_file_list
#define dev_add_param_uuid		bobject_add_param_uuid
#define dev_add_param_guid		bobject_add_param_guid
#define dev_param_set_generic		bobject_param_set_generic

#define dev_add_param_int		bobject_add_param_int
#define dev_add_param_int32		bobject_add_param_int32
#define dev_add_param_uint32		bobject_add_param_uint32
#define dev_add_param_int64		bobject_add_param_int64
#define dev_add_param_uint64		bobject_add_param_uint64

#define dev_add_param_int_fixed		bobject_add_param_int_fixed
#define dev_add_param_int32_fixed	bobject_add_param_int32_fixed
#define dev_add_param_uint32_fixed	bobject_add_param_uint32_fixed
#define dev_add_param_int64_fixed	bobject_add_param_int64_fixed
#define dev_add_param_uint64_fixed	bobject_add_param_uint64_fixed


#define dev_add_param_int_ro		bobject_add_param_int_ro
#define dev_add_param_int32_ro		bobject_add_param_int32_ro
#define dev_add_param_uint32_ro		bobject_add_param_uint32_ro
#define dev_add_param_int64_ro		bobject_add_param_int64_ro
#define dev_add_param_uint64_ro		bobject_add_param_uint64_ro

#define dev_add_param_bool		bobject_add_param_bool
#define dev_add_param_bool_fixed	bobject_add_param_bool_fixed
#define dev_add_param_bool_ro		bobject_add_param_bool_ro
#define dev_add_param_string_ro		bobject_add_param_string_ro
#define dev_add_param_string_fixed	bobject_add_param_string_fixed
#define dev_add_param_enum_ro		bobject_add_param_enum_ro
#define dev_add_param_bitmask_ro	bobject_add_param_bitmask_ro

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
