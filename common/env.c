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
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <init.h>

/**
 * Managment of a environment variable
 */
struct variable_d {
	/*! List management */
	struct variable_d *next;
	/*! variable length data */
	char data[0];
};

#define VARIABLE_D_SIZE(name, value) (sizeof(struct variable_d) + strlen(name) + strlen(value) + 2)

/**
 * FIXME
 */
struct env_context {
	/*! FIXME */
	struct env_context *parent;
	/*! FIXME */
	struct variable_d *local;
	/*! FIXME */
	struct variable_d *global;
};

static struct env_context *context;

/**
 * Remove a list of environment variables
 * @param[inout] v Variable anchor to remove
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

late_initcall(env_push_context);

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
static char *var_val(struct variable_d *var)
{
	return &var->data[strlen(var->data) + 1];
}

/**
 * Return variable's name
 * @param[in] var Variable of interest
 * @return Name as text
 */
static char *var_name(struct variable_d *var)
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
		const char *ret = 0;
		char *devstr = strdup(name);
		char *par = strchr(devstr, '.');
		struct device_d *dev;
		*par = 0;
		dev = get_device_by_id(devstr);
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

int setenv(const char *name, const char *value)
{
	char *par;

	if (!*value)
		value = NULL;

	if ((par = strchr(name, '.'))) {
		struct device_d *dev;
		*par++ = 0;
		dev = get_device_by_id(name);
		if (dev) {
			int ret = dev_set_param(dev, par, value);
			if (ret < 0)
				perror("set parameter");
			errno = 0;
		} else {
			errno = -ENODEV;
			perror("set parameter");
		}
		return errno;
	}

	if (getenv_raw(context->global, name))
		setenv_raw(context->global, name, value);
	else
		setenv_raw(context->local, name, value);

	return 0;
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

static int do_printenv (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	struct variable_d *var;
	struct env_context *c;

	if (argc == 2) {
		const char *val = getenv(argv[1]);
		if (val) {
			printf("%s=%s\n", argv[1], val);
			return 0;
		}
		printf("## Error: \"%s\" not defined\n", argv[1]);
		return -EINVAL;
	}

	var = context->local->next;
	printf("locals:\n");
	while (var) {
		printf("%s=%s\n", var_name(var), var_val(var));
		var = var->next;
	}

	printf("globals:\n");
	c = context;
	while(c) {
		var = c->global->next;
		while (var) {
			printf("%s=%s\n", var_name(var), var_val(var));
			var = var->next;
		}
		c = c->parent;
	}

	return 0;
}

U_BOOT_CMD_START(printenv)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_printenv,
	.usage		= "print environment variables",
	U_BOOT_CMD_HELP(
	"\n    - print values of all environment variables\n"
	"printenv name ...\n"
	"    - print value of environment variable 'name'\n")
U_BOOT_CMD_END

/**
 * @page printenv_command printenv
 *
 * Usage: printenv [<name>]
 *
 * Print environment variables.
 * If <name> was given, it prints out its content if the environment variable
 * <name> exists.
 * Without the <name> argument all current environment variables are printed.
 */

#ifdef CONFIG_SIMPLE_PARSER
static int do_setenv ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	setenv(argv[1], argv[2]);

	return 0;
}

U_BOOT_CMD_START(setenv)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_setenv,
	.usage		= "set environment variables",
	U_BOOT_CMD_HELP(
	"name value ...\n"
	"    - set environment variable 'name' to 'value ...'\n"
	"setenv name\n"
	"    - delete environment variable 'name'\n")
U_BOOT_CMD_END

/**
 * @page setenv_command setenv
 *
 * Usage: setenv <name> [<value>]
 *
 * Set environment variable <name> to <value ...>
 * If no <value> was given, the variable <name> will be removed.
 *
 * This command can be replaced by using the simpler form:
 *
 * <name> = <value>
 */

#endif

static int do_export ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i = 1;
	char *ptr;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	while (i < argc) {
		if ((ptr = strchr(argv[i], '='))) {
			*ptr++ = 0;
			setenv(argv[i], ptr);
		}
		if (export(argv[i])) {
			printf("could not export: %s\n", argv[i]);
			return 1;
		}
		i++;
	}

	return 0;
}

static __maybe_unused char cmd_export_help[] =
"Usage: export <var>[=value]...\n"
"export an environment variable to subsequently executed scripts\n";

U_BOOT_CMD_START(export)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_export,
	.usage		= "export environment variables",
	U_BOOT_CMD_HELP(cmd_export_help)
U_BOOT_CMD_END

/**
 * @page export_command export
 *
 * Usage: export <var>[=value]...
 *
 * Export an environment variable to subsequently executed scripts
 */

