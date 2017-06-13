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

int globalvar_add_simple_string(const char *name, char **value);
int globalvar_add_simple_int(const char *name, int *value,
			     const char *format);
int globalvar_add_simple_bool(const char *name, int *value);
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

static inline int globalvar_add_simple_string(const char *name, char **value)
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

void nv_var_set_clean(void);
int nvvar_save(void);
int nv_global_complete(struct string_list *sl, char *instr);

#endif /* __GLOBALVAR_H */
