/*
 * env.c - environment variables storage
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <environment.h>

#define VARIABLE_D_SIZE(name, value) (sizeof(struct variable_d) + strlen(name) + strlen(value) + 2)

static struct env_context *context;

/**
 * Remove a list of environment variables
 * @param[in] v Variable anchor to remove
 */
static void free_variables(struct variable_d *v)
{
	struct variable_d *next;

	while (v) {
		next = v->next;
		free(v);
		v = next;
	}
}

/** Read back current context */
struct env_context *get_current_context(void)
{
	return context;
}
EXPORT_SYMBOL(get_current_context);


/**
 * FIXME
 */
int env_push_context(void)
{
	struct env_context *c = xzalloc(sizeof(struct env_context));

	c->local = xzalloc(VARIABLE_D_SIZE("", ""));
	c->global = xzalloc(VARIABLE_D_SIZE("", ""));

	if (!context) {
		context = c;
		return 0;
	}

	c->parent = context;
	context = c;

	return 0;
}

device_initcall(env_push_context);

/**
 * FIXME
 */
int env_pop_context(void)
{
	struct env_context *c = context;

	if (context->parent) {
		c = context->parent;
		free_variables(context->local);
		free_variables(context->global);
		free(context);
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
	return &var->data[strlen(var->data) + 1];
}

/**
 * Return variable's name
 * @param[in] var Variable of interest
 * @return Name as text
 */
char *var_name(struct variable_d *var)
{
	return var->data;
}

static const char *getenv_raw(struct variable_d *var, const char *name)
{
	while (var) {
		if (!strcmp(var_name(var), name))
			return var_val(var);
		var = var->next;
	}
	return NULL;
}

const char *getenv (const char *name)
{
	struct env_context *c;
	const char *val;

	if (strchr(name, '.')) {
		const char *ret = NULL;
		char *devstr = strdup(name);
		char *par = strchr(devstr, '.');
		struct device_d *dev;
		*par = 0;
		dev = get_device_by_name(devstr);
		if (dev) {
				par++;
				ret = dev_get_param(dev, par);
		}
		free(devstr);
		return ret;
	}

	c = context;

	val = getenv_raw(c->local, name);
	if (val)
		return val;

	while (c) {
		val = getenv_raw(c->global, name);
		if (val)
			return val;
		c = c->parent;
	}
	return NULL;
}
EXPORT_SYMBOL(getenv);

static int setenv_raw(struct variable_d *var, const char *name, const char *value)
{
	struct variable_d *newvar = NULL;

	if (value) {
		newvar = xzalloc(VARIABLE_D_SIZE(name, value));
		strcpy(&newvar->data[0], name);
		strcpy(&newvar->data[strlen(name) + 1], value);
	}

	while (var->next) {
		if (!strcmp(var->next->data, name)) {
			if (value) {
				newvar->next = var->next->next;
				free(var->next);
				var->next = newvar;
				return 0;
			} else {
				struct variable_d *tmp;
				tmp = var->next;
				var->next = var->next->next;
				free(tmp);
				return 0;
			}
		}
		var = var->next;
	}
	var->next = newvar;
	return 0;
}

int setenv(const char *_name, const char *value)
{
	char *name = strdup(_name);
	char *par;
	struct variable_d *var;
	int ret = 0;

	if (value && !*value)
		value = NULL;


	if ((par = strchr(name, '.'))) {
		struct device_d *dev;

		*par++ = 0;
		dev = get_device_by_name(name);
		if (dev)
			ret = dev_set_param(dev, par, value);
		else
			ret = -ENODEV;

		errno = ret;

		if (ret < 0)
			perror("set parameter");

		goto out;
	}

	if (getenv_raw(context->global, name))
		var = context->global;
	else
		var = context->local;

	ret = setenv_raw(var, name, value);
out:
	free(name);

	return ret;
}
EXPORT_SYMBOL(setenv);

int export(const char *varname)
{
	const char *val = getenv_raw(context->local, varname);

	if (val) {
		setenv_raw(context->global, varname, val);
		setenv_raw(context->local, varname, NULL);
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL(export);
