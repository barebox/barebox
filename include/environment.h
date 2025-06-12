/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

#include <linux/list.h>
#include <errno.h>

/**
 * Managment of a environment variable
 */
struct variable_d {
	struct list_head list;
	const char *name;
	const char *data;
};

struct env_context {
	struct env_context *parent;
	struct list_head local;
	struct list_head global;
};

struct env_context *get_current_context(void);
const char *var_val(struct variable_d *);
const char *var_name(struct variable_d *);

#if IS_ENABLED(CONFIG_ENVIRONMENT_VARIABLES) && IN_PROPER
const char *getenv(const char *);
int envvar_for_each(int (*fn)(struct variable_d *v, void *data), void *data);
int setenv(const char *, const char *);
int pr_setenv(const char *, const char *fmt, ...) __printf(2, 3);
void export_env_ull(const char *name, unsigned long long val);
int getenv_ull(const char *name, unsigned long long *val);
int getenv_ullx(const char *name, unsigned long long *val);
int getenv_ul(const char *name, unsigned long *val);
int getenv_uint(const char *name, unsigned int *val);
int getenv_bool(const char *var, int *val);
const char *getenv_nonempty(const char *var);
#else
static inline char *getenv(const char *var)
{
	return NULL;
}

static inline int envvar_for_each(int (*fn)(struct list_head *l, void *data),
				  void *data)
{
	return 0;
}

static inline int setenv(const char *var, const char *val)
{
	return 0;
}

static inline __printf(2, 3) int pr_setenv(const char *var, const char *fmt, ...)
{
	return 0;
}

static inline void export_env_ull(const char *name, unsigned long long val) {}

static inline int getenv_ull(const char *name, unsigned long long *val)
{
	return -EINVAL;
}

static inline int getenv_ullx(const char *name, unsigned long long *val)
{
	return -EINVAL;
}

static inline int getenv_ul(const char *name, unsigned long *val)
{
	return -EINVAL;
}

static inline int getenv_uint(const char *name, unsigned int *val)
{
	return -EINVAL;
}

static inline int export(const char *var)
{
	return -EINVAL;
}

static inline int getenv_bool(const char *var, int *val)
{
	return -EINVAL;
}

static inline const char *getenv_nonempty(const char *var)
{
	return NULL;
}
#endif

int env_pop_context(void);
int env_push_context(void);

int export(const char *);

static inline int unsetenv(const char *var)
{
	return setenv(var, NULL);
}

#endif	/* _ENVIRONMENT_H_ */

/**
 * @file
 * @brief Environment handling
 *
 * Important: This file will also be used on the host to create
 * the default environment when building the barebox binary.
 */
