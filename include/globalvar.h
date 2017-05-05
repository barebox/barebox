#ifndef __GLOBALVAR_H
#define __GLOBALVAR_H

#include <param.h>
#include <driver.h>
#include <linux/err.h>
#include <stringlist.h>

extern struct device_d global_device;

#ifdef CONFIG_GLOBALVAR
int globalvar_add_simple(const char *name, const char *value);

void globalvar_remove(const char *name);
char *globalvar_get_match(const char *match, const char *separator);
void globalvar_set_match(const char *match, const char *val);

int __globalvar_add_simple_string(const char *name, char **value);
int __globalvar_add_simple_int(const char *name, void *value,
			       enum param_type type, const char *format);
int globalvar_add_simple_enum(const char *name,	int *value,
			      const char * const *names, int max);
int globalvar_add_simple_bitmask(const char *name, unsigned long *value,
				 const char * const *names, int max);
int globalvar_add_simple_ip(const char *name, IPaddr_t *ip);

int nvvar_load(void);
void nvvar_print(void);
int nvvar_add(const char *name, const char *value);
int nvvar_remove(const char *name);
void globalvar_print(void);

void dev_param_init_from_nv(struct device_d *dev, const char *name);

#else
static inline int globalvar_add_simple(const char *name, const char *value)
{
	return 0;
}

static inline int __globalvar_add_simple_int(const char *name, void *value,
			       enum param_type type, const char *format)
{
	return 0;
}

static inline int __globalvar_add_simple_string(const char *name, char **value)
{
	return 0;
}

static inline int globalvar_add_simple_enum(const char *name,
		int *value, const char * const *names, int max)
{
	return 0;
}

static inline int globalvar_add_simple_bitmask(const char *name,
					       unsigned long *value,
					       const char * const *names,
					       int max)
{
	return 0;
}

static inline int globalvar_add_simple_ip(const char *name,
		IPaddr_t *ip)
{
	return 0;
}

static inline void globalvar_remove(const char *name) {}

static inline void globalvar_print(void) {}

static inline char *globalvar_get_match(const char *match, const char *separator)
{
	return NULL;
}

static inline void globalvar_set_match(const char *match, const char *val) {}

static inline int nvvar_load(void)
{
	return 0;
}

static inline void nvvar_print(void) {}

static inline int nvvar_add(const char *name, const char *value)
{
	return 0;
}

static inline int nvvar_remove(const char *name)
{
	return 0;
}

static inline int nvvar_save(void)
{
	return 0;
}

static inline void dev_param_init_from_nv(struct device_d *dev, const char *name)
{
}

#endif

#define DECLARE_GLOBALVAR_INT(intname, inttype, paramtype) \
	static inline int globalvar_add_simple_##intname(const char *name,	\
						      inttype *value,		\
						      const char *format)	\
	{									\
		return __globalvar_add_simple_int(name, value,			\
						  paramtype,			\
						  format);			\
	}

DECLARE_GLOBALVAR_INT(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_GLOBALVAR_INT(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_GLOBALVAR_INT(uint64, uint64_t, PARAM_TYPE_UINT64)
DECLARE_GLOBALVAR_INT(int64, int64_t, PARAM_TYPE_INT64)

static inline int globalvar_add_simple_bool(const char *name, uint32_t *value)
{
	return __globalvar_add_simple_int(name, value, PARAM_TYPE_BOOL, "%u");
}

static inline int globalvar_add_simple_string(const char *name, char **value)
{
	return __globalvar_add_simple_string(name, value);
}

#define DECLARE_GLOBALVAR_INT_RO(intname, inttype, paramtype) \
	static inline int globalvar_add_simple_##intname##_ro(const char *name,	\
						      inttype *value,		\
						      const char *format)	\
	{									\
		return PTR_ERR_OR_ZERO(__dev_add_param_int(&global_device, name,\
							   param_set_readonly,	\
							   NULL, value,		\
							   paramtype,		\
							   format, NULL));	\
	}

DECLARE_GLOBALVAR_INT_RO(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_GLOBALVAR_INT_RO(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_GLOBALVAR_INT_RO(uint64, uint64_t, PARAM_TYPE_UINT64)
DECLARE_GLOBALVAR_INT_RO(int64, int64_t, PARAM_TYPE_INT64)

static inline int globalvar_add_simple_bool_ro(const char *name, uint32_t *value)
{
	return PTR_ERR_OR_ZERO(__dev_add_param_int(&global_device, name,
						   param_set_readonly, NULL,
						   value, PARAM_TYPE_BOOL, "%u",
						   NULL));
}

static inline int globalvar_add_simple_string_ro(const char *name, char **value)
{
	return __globalvar_add_simple_string(name, value);
}

#define DECLARE_GLOBALVAR_INT_FIXED(intname, inttype, paramtype) \
	static inline int globalvar_add_simple_##intname##_fixed(const char *name,	\
								 inttype value,		\
								 const char *format)	\
	{										\
		return PTR_ERR_OR_ZERO(__dev_add_param_int(&global_device, name,	\
							   ERR_PTR(-EROFS), NULL,	\
							   &value, paramtype,		\
							   format, NULL));		\
	}

DECLARE_GLOBALVAR_INT_FIXED(uint32, uint32_t, PARAM_TYPE_UINT32)
DECLARE_GLOBALVAR_INT_FIXED(int32, int32_t, PARAM_TYPE_INT32)
DECLARE_GLOBALVAR_INT_FIXED(uint64, uint64_t, PARAM_TYPE_UINT64)
DECLARE_GLOBALVAR_INT_FIXED(int64, int64_t, PARAM_TYPE_INT64)

static inline int globalvar_add_simple_bool_fixed(const char *name, uint32_t value)
{
	return PTR_ERR_OR_ZERO(__dev_add_param_int(&global_device, name, ERR_PTR(-EROFS),
						   NULL, &value, PARAM_TYPE_BOOL, "%u",
						   NULL));
}

static inline int globalvar_add_simple_string_fixed(const char *name, char *value)
{
	return PTR_ERR_OR_ZERO(dev_add_param_string_fixed(&global_device, name, value));
}

static inline int globalvar_add_simple_enum_ro(const char *name, int *value,
					       const char * const *names, int max)
{
	return PTR_ERR_OR_ZERO(dev_add_param_enum_ro(&global_device, name, value, names,
						     max));
}

void nv_var_set_clean(void);
int nvvar_save(void);
int nv_global_complete(struct string_list *sl, char *instr);

#endif /* __GLOBALVAR_H */
