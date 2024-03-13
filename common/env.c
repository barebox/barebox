// SPDX-License-Identifier: GPL-2.0-only
/*
 * env.c - environment variables storage
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

/**
 * @file
 * @brief Environment support
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>
#include <string.h>
#include <environment.h>

static struct env_context root = {
	.local = LIST_HEAD_INIT(root.local),
	.global = LIST_HEAD_INIT(root.global),
};

static struct env_context *context = &root;

/**
 * Remove a list of environment variables
 * @param[in] v Variable anchor to remove
 */
static void free_context(struct env_context *c)
{
	struct variable_d *v, *tmp;

	list_for_each_entry_safe(v, tmp, &c->local, list) {
		free(v->name);
		free(v->data);
		list_del(&v->list);
		free(v);
	}

	list_for_each_entry_safe(v, tmp, &c->global, list) {
		free(v->name);
		free(v->data);
		list_del(&v->list);
		free(v);
	}

	free(c);
}

/** Read back current context */
struct env_context *get_current_context(void)
{
	return context;
}
EXPORT_SYMBOL(get_current_context);


/**
 * Create a new variable context and put it on the stack
 */
int env_push_context(void)
{
	struct env_context *c = xzalloc(sizeof(struct env_context));

	INIT_LIST_HEAD(&c->local);
	INIT_LIST_HEAD(&c->global);

	c->parent = context;
	context = c;

	return 0;
}

/**
 * free current variable context and restore the previous one
 */
int env_pop_context(void)
{
	struct env_context *c;

	if (context->parent) {
		c = context->parent;
		free_context(context);
		context = c;
		return 0;
	}
	return -EINVAL;
}

/**
 * Return variable's value
 * @param[in] var Variable of interest
 * @return Value as text
 */
char *var_val(struct variable_d *var)
{
	return var->data;
}

/**
 * Return variable's name
 * @param[in] var Variable of interest
 * @return Name as text
 */
char *var_name(struct variable_d *var)
{
	return var->name;
}

static const char *getenv_raw(struct list_head *l, const char *name)
{
	struct variable_d *v;

	list_for_each_entry(v, l, list) {
		if (!strcmp(var_name(v), name))
			return var_val(v);
	}

	return NULL;
}

static const char *dev_getenv(const char *name)
{
	const char *pos, *val, *dot, *varname;
	char *devname;
	struct device *dev;

	pos = name;

	while (1) {
		dot = strchr(pos, '.');
		if (!dot)
			break;

		devname = xstrndup(name, dot - name);
		varname = dot + 1;

		dev = get_device_by_name(devname);

		free(devname);

		if (dev) {
			val = dev_get_param(dev, varname);
			if (val)
				return val;
		}

		pos = dot + 1;
	}

	return NULL;
}

const char *getenv(const char *name)
{
	struct env_context *c;
	const char *val;

	if (strchr(name, '.'))
		return dev_getenv(name);

	c = context;

	val = getenv_raw(&c->local, name);
	if (val)
		return val;

	while (c) {
		val = getenv_raw(&c->global, name);
		if (val)
			return val;
		c = c->parent;
	}
	return NULL;
}
EXPORT_SYMBOL(getenv);

static int setenv_raw(struct list_head *l, const char *name, const char *value)
{
	struct variable_d *v;

	list_for_each_entry(v, l, list) {
		if (!strcmp(v->name, name)) {
			if (value) {
				free(v->data);
				v->data = xstrdup(value);

				return 0;
			} else {
				list_del(&v->list);
				free(v->name);
				free(v->data);
				free(v);

				return 0;
			}
		}
	}

	if (value) {
		v = xzalloc(sizeof(*v));
		v->name = xstrdup(name);
		v->data = xstrdup(value);
		list_add_tail(&v->list, l);
	}

	return 0;
}

static int dev_setenv(const char *name, const char *val)
{
	const char *pos, *dot, *varname;
	char *devname;
	struct device *dev;

	pos = name;

	while (1) {
		dot = strchr(pos, '.');
		if (!dot)
			break;

		devname = xstrndup(name, dot - name);
		varname = dot + 1;

		dev = get_device_by_name(devname);

		free(devname);

		if (dev) {
			if (get_param_by_name(dev, varname))
				return dev_set_param(dev, varname, val);
		}

		pos = dot + 1;
	}

	return -ENODEV;
}

/**
 * setenv - set environment variables
 * @name - Variable name
 * @value - the value to set, empty string not handled specially
 *
 * Returns 0 for success and a negative error code otherwise
 * Use unsetenv() to unset.
 */
int setenv(const char *_name, const char *value)
{
	char *name = strdup(_name);
	int ret = 0;
	struct list_head *list;

	if (strchr(name, '.')) {
		ret = dev_setenv(name, value);
		if (ret)
			eprintf("Cannot set parameter %s: %s\n", name, strerror(-ret));
		goto out;
	}

	if (getenv_raw(&context->global, name))
		list = &context->global;
	else
		list = &context->local;

	ret = setenv_raw(list, name, value);
out:
	free(name);

	return ret;
}
EXPORT_SYMBOL(setenv);

/**
 * pr_setenv - set environment variables
 * @name - Variable name
 * @fmt - the format string to use
 *
 * Returns 0 for success and a negative error code otherwise
 * Use unsetenv() to unset.
 */
int pr_setenv(const char *name, const char *fmt, ...)
{
	va_list ap;
	int ret = 0;
	char *value;
	int len;

	va_start(ap, fmt);
	len = vasprintf(&value, fmt, ap);
	va_end(ap);

	if (len < 0)
		return -ENOMEM;

	ret = setenv(name, value);
	free(value);

	return ret;
}
EXPORT_SYMBOL(pr_setenv);

int export(const char *varname)
{
	const char *val = getenv_raw(&context->local, varname);

	if (val) {
		setenv_raw(&context->global, varname, val);
		setenv_raw(&context->local, varname, NULL);
	}
	return 0;
}
EXPORT_SYMBOL(export);

void export_env_ull(const char *name, unsigned long long val)
{
	char *valstr = basprintf("%llu", val);

	setenv(name, valstr);
	export(name);

	kfree(valstr);
}
EXPORT_SYMBOL(export_env_ull);

/*
 * Like regular getenv, but never returns an empty string.
 * If the string is empty, NULL is returned instead
 */
const char *getenv_nonempty(const char *var)
{
	const char *val = getenv(var);

	if (isempty(val))
		return NULL;

	return val;
}
EXPORT_SYMBOL(getenv_nonempty);

static int getenv_ull_base(const char *var, int base, unsigned long long *val)
{
	const char *valstr = getenv(var);

	if (isempty(valstr))
		return -EINVAL;

	*val = simple_strtoull(valstr, NULL, base);

	return 0;
}

int getenv_ull(const char *var , unsigned long long *val)
{
	return getenv_ull_base(var, 0, val);
}
EXPORT_SYMBOL(getenv_ull);

int getenv_ullx(const char *var , unsigned long long *val)
{
	return getenv_ull_base(var, 16, val);
}
EXPORT_SYMBOL(getenv_ullx);

int getenv_ul(const char *var , unsigned long *val)
{
	const char *valstr = getenv(var);

	if (isempty(valstr))
		return -EINVAL;

	*val = simple_strtoul(valstr, NULL, 0);

	return 0;
}
EXPORT_SYMBOL(getenv_ul);

int getenv_uint(const char *var , unsigned int *val)
{
	const char *valstr = getenv(var);

	if (isempty(valstr))
		return -EINVAL;

	*val = simple_strtoul(valstr, NULL, 0);

	return 0;
}
EXPORT_SYMBOL(getenv_uint);

/**
 * getenv_bool - get a boolean value from an environment variable
 * @var - Variable name
 * @val - The boolean value returned.
 *
 * This function treats
 * - any positive (nonzero) number as true
 * - "0" as false
 * - "true" (case insensitive) as true
 * - "false" (case insensitive) as false
 *
 * Returns 0 for success or negative error code if the variable does
 * not exist or contains something this function does not recognize
 * as true or false.
 */
int getenv_bool(const char *var, int *val)
{
	const char *valstr = getenv(var);

	return strtobool(valstr, val);
}
EXPORT_SYMBOL(getenv_bool);
