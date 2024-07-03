/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __GLOBALVAR_H
#define __GLOBALVAR_H

#include <param.h>
#include <driver.h>
#include <linux/err.h>
#include <stringlist.h>

extern struct device global_device;

#ifdef CONFIG_GLOBALVAR

static inline const char *globalvar_get(const char *name)
{
	return dev_get_param(&global_device, name);
}

int globalvar_add_simple(const char *name, const char *value);

void globalvar_remove(const char *name);
char *globalvar_get_match(const char *match, const char *separator);
void globalvar_set_match(const char *match, const char *val);
void globalvar_set(const char *name, const char *val);

int globalvar_add_simple_string(const char *name, char **value);
int globalvar_add_simple_int(const char *name, int *value,
			     const char *format);
int globalvar_add_simple_uint64(const char *name, u64 *value,
				const char *format);
int globalvar_add_bool(const char *name,
		       int (*set)(struct param_d *, void *),
		       int *value, void *priv);
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

void dev_param_init_from_nv(struct device *dev, const char *name);
void globalvar_alias_deprecated(const char *oldname, const char *newname);

#else
static inline const char *globalvar_get(const char *name)
{
	return NULL;
}

static inline int globalvar_add_simple(const char *name, const char *value)
{
	return 0;
}

static inline int globalvar_add_simple_string(const char *name, char **value)
{
	return 0;
}

static inline int globalvar_add_simple_int(const char *name,
		int *value, const char *format)
{
	return 0;
}

static inline int globalvar_add_simple_uint64(const char *name,
		u64 *value, const char *format)
{
	return 0;
}

static inline int globalvar_add_bool(const char *name,
		int (*set)(struct param_d *, void *),
		int *value, void *priv)
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

static inline void globalvar_set(const char *name, const char *val) {}

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

static inline void dev_param_init_from_nv(struct device *dev,
					  const char *name)
{
}

static inline void globalvar_alias_deprecated(const char *newname, const char *oldname)
{
}

#endif

void nv_var_set_clean(void);
int nvvar_save(void);
int nv_complete(struct string_list *sl, char *instr);
int global_complete(struct string_list *sl, char *instr);

static inline int globalvar_add_simple_bool(const char *name, int *value)
{
	return globalvar_add_bool(name, NULL, value, NULL);
}

#endif /* __GLOBALVAR_H */
