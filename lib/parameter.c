/*
 * parameter.c - device parameters
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
 */

/**
 * @file
 * @brief Handling device specific parameters
 */
#include <common.h>
#include <param.h>
#include <errno.h>
#include <net.h>
#include <malloc.h>
#include <driver.h>
#include <linux/err.h>

struct param_d *get_param_by_name(struct device_d *dev, const char *name)
{
	struct param_d *p;

	list_for_each_entry(p, &dev->parameters, list) {
		if (!strcmp(p->name, name))
			return p;
	}

	return NULL;
}

/**
 * dev_get_param - get the value of a parameter
 * @param dev	The device
 * @param name	The name of the parameter
 * @return	The value
 */
const char *dev_get_param(struct device_d *dev, const char *name)
{
	struct param_d *param = get_param_by_name(dev, name);

	if (!param) {
		errno = EINVAL;
		return NULL;
	}

	return param->get(dev, param);
}

/**
 * dev_set_param - set a parameter of a device to a new value
 * @param dev	The device
 * @param name	The name of the parameter
 * @param val	The new value of the parameter
 */
int dev_set_param(struct device_d *dev, const char *name, const char *val)
{
	struct param_d *param;
	int ret;

	if (!dev) {
		errno = ENODEV;
		return -ENODEV;
	}

	param = get_param_by_name(dev, name);

	if (!param) {
		errno = EINVAL;
		return -EINVAL;
	}

	if (param->flags & PARAM_FLAG_RO) {
		errno = EACCES;
		return -EACCES;
	}

	ret = param->set(dev, param, val);
	if (ret)
		errno = -ret;

	return ret;
}

/**
 * dev_param_set_generic - generic setter function for a parameter
 * @param dev	The device
 * @param p	the parameter
 * @param val	The new value
 *
 * If used the value of a parameter is a string allocated with
 * malloc and freed with free. If val is NULL the value is freed. This is
 * used during deregistration of the parameter to free the alloctated
 * memory.
 */
int dev_param_set_generic(struct device_d *dev, struct param_d *p,
		const char *val)
{
	if (p->value)
		free(p->value);
	if (!val) {
		p->value = NULL;
		return 0;
	}
	p->value = strdup(val);
	return 0;
}

static const char *param_get_generic(struct device_d *dev, struct param_d *p)
{
	return p->value ? p->value : "";
}

static int __dev_add_param(struct param_d *param, struct device_d *dev, const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *dev, struct param_d *p),
		unsigned long flags)
{
	if (get_param_by_name(dev, name))
		return -EEXIST;

	if (set)
		param->set = set;
	else
		param->set = dev_param_set_generic;
	if (get)
		param->get = get;
	else
		param->get = param_get_generic;

	param->name = strdup(name);
	param->flags = flags;
	param->dev = dev;
	list_add_tail(&param->list, &dev->parameters);

	return 0;
}

/**
 * dev_add_param - add a parameter to a device
 * @param dev	The device
 * @param name	The name of the parameter
 * @param set	setter function for the parameter
 * @param get	getter function for the parameter
 * @param flags
 *
 * This function adds a new parameter to a device. The get/set functions can
 * be zero in which case the generic functions are used. The generic functions
 * expect the parameter value to be a string which can be freed with free(). Do
 * not use static arrays when using the generic functions.
 */
int dev_add_param(struct device_d *dev, const char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		const char *(*get)(struct device_d *dev, struct param_d *param),
		unsigned long flags)
{
	struct param_d *param;
	int ret;

	param = xzalloc(sizeof(*param));

	ret = __dev_add_param(param, dev, name, set, get, flags);
	if (ret)
		free(param);

	return ret;
}

/**
 * dev_add_param_fixed - add a readonly parameter to a device
 * @param dev	The device
 * @param name	The name of the parameter
 * @param value	The value of the parameter
 */
int dev_add_param_fixed(struct device_d *dev, char *name, char *value)
{
	struct param_d *param;
	int ret;

	param = xzalloc(sizeof(*param));

	ret = __dev_add_param(param, dev, name, NULL, NULL, PARAM_FLAG_RO);
	if (ret) {
		free(param);
		return ret;
	}

	param->value = strdup(value);

	return 0;
}

struct param_int {
	struct param_d param;
	int *value;
	const char *format;
#define PARAM_INT_FLAG_BOOL (1 << 0)
	unsigned flags;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

static inline struct param_int *to_param_int(struct param_d *p)
{
	return container_of(p, struct param_int, param);
}

static int param_int_set(struct device_d *dev, struct param_d *p, const char *val)
{
	struct param_int *pi = to_param_int(p);
	int value_save = *pi->value;
	int ret;

	if (!val)
		return -EINVAL;

	*pi->value = simple_strtol(val, NULL, 0);

	if (pi->flags & PARAM_INT_FLAG_BOOL)
		*pi->value = !!*pi->value;

	if (!pi->set)
		return 0;

	ret = pi->set(p, p->driver_priv);
	if (ret)
		*pi->value = value_save;

	return ret;
}

static const char *param_int_get(struct device_d *dev, struct param_d *p)
{
	struct param_int *pi = to_param_int(p);
	int ret;

	if (pi->get) {
		ret = pi->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	free(p->value);
	p->value = asprintf(pi->format, *pi->value);

	return p->value;
}

/**
 * dev_add_param_int - add an integer parameter to a device
 * @param dev	The device
 * @param name	The name of the parameter
 * @param set	set function
 * @param get	get function
 * @param value	pointer to the integer containing the value of the parameter
 * @param format the printf format used to print the value
 * @param priv	user private data, will be passed to get/set
 *
 * The get function can be used as a notifier when the variable is about
 * to be read.
 * The set function can be used as a notifer when the variable is about
 * to be written. Can also be used to limit the value.
 */
struct param_d *dev_add_param_int(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char *format, void *priv)
{
	struct param_int *pi;
	struct param_d *p;
	int ret;

	pi = xzalloc(sizeof(*pi));
	pi->value = value;
	pi->format = format;
	pi->set = set;
	pi->get = get;
	p = &pi->param;
	p->driver_priv = priv;

	ret = __dev_add_param(p, dev, name, param_int_set, param_int_get, 0);
	if (ret) {
		free(pi);
		return ERR_PTR(ret);
	}

	return &pi->param;
}

struct param_enum {
	struct param_d param;
	int *value;
	const char **names;
	int num_names;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

static inline struct param_enum *to_param_enum(struct param_d *p)
{
	return container_of(p, struct param_enum, param);
}

static int param_enum_set(struct device_d *dev, struct param_d *p, const char *val)
{
	struct param_enum *pe = to_param_enum(p);
	int value_save = *pe->value;
	int i, ret;

	if (!val)
		return -EINVAL;

	for (i = 0; i < pe->num_names; i++)
		if (pe->names[i] && !strcmp(val, pe->names[i]))
			break;

	if (i == pe->num_names)
		return -EINVAL;

	*pe->value = i;

	if (!pe->set)
		return 0;

	ret = pe->set(p, p->driver_priv);
	if (ret)
		*pe->value = value_save;

	return ret;
}

static const char *param_enum_get(struct device_d *dev, struct param_d *p)
{
	struct param_enum *pe = to_param_enum(p);
	int ret;

	if (pe->get) {
		ret = pe->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	free(p->value);
	p->value = strdup(pe->names[*pe->value]);

	return p->value;
}

static void param_enum_info(struct param_d *p)
{
	struct param_enum *pe = to_param_enum(p);
	int i;

	printf(" (");

	for (i = 0; i < pe->num_names; i++) {
		if (!pe->names[i] || !*pe->names[i])
			continue;
		printf("\"%s\"%s", pe->names[i],
				i ==  pe->num_names - 1 ? ")" : ", ");
	}
}

struct param_d *dev_add_param_enum(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, const char **names, int num_names, void *priv)
{
	struct param_enum *pe;
	struct param_d *p;
	int ret;

	pe = xzalloc(sizeof(*pe));

	pe->value = value;
	pe->set = set;
	pe->get = get;
	pe->names = names;
	pe->num_names = num_names;
	p = &pe->param;
	p->driver_priv = priv;

	ret = __dev_add_param(p, dev, name, param_enum_set, param_enum_get, 0);
	if (ret) {
		free(pe);
		return ERR_PTR(ret);
	}

	p->info = param_enum_info;

	return &pe->param;
}

/**
 * dev_add_param_bool - add an boolean parameter to a device
 * @param dev	The device
 * @param name	The name of the parameter
 * @param set	set function
 * @param get	get function
 * @param value	pointer to the integer containing the value of the parameter
 * @param priv	user private data, will be passed to get/set
 *
 * The get function can be used as a notifier when the variable is about
 * to be read.
 * The set function can be used as a notifer when the variable is about
 * to be written. Can also be used to limit the value.
 */
struct param_d *dev_add_param_bool(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, void *priv)
{
	struct param_int *pi;
	struct param_d *p;

	p = dev_add_param_int(dev, name, set, get, value, "%d", priv);
	if (IS_ERR(p))
		return p;

	pi = to_param_int(p);
	pi->flags |= PARAM_INT_FLAG_BOOL;

	return p;
}

struct param_int_ro {
	struct param_d param;
	char *value;
};

/**
 * dev_add_param_int_ro - add a read only integer parameter to a device
 * @param dev	The device
 * @param name	The name of the parameter
 * @param value	The value of the parameter
 * @param format the printf format used to print the value
 */
struct param_d *dev_add_param_int_ro(struct device_d *dev, const char *name,
		int value, const char *format)
{
	struct param_int *piro;
	int ret;

	piro = xzalloc(sizeof(*piro));

	ret = __dev_add_param(&piro->param, dev, name, NULL, NULL, 0);
	if (ret) {
		free(piro);
		return ERR_PTR(ret);
	}

	piro->param.value = asprintf(format, value);

	return &piro->param;
}

#ifdef CONFIG_NET
struct param_ip {
	struct param_d param;
	IPaddr_t *ip;
	const char *format;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

static inline struct param_ip *to_param_ip(struct param_d *p)
{
	return container_of(p, struct param_ip, param);
}

static int param_ip_set(struct device_d *dev, struct param_d *p, const char *val)
{
	struct param_ip *pi = to_param_ip(p);
	IPaddr_t ip_save = *pi->ip;
	int ret;

	if (!val)
		return -EINVAL;

	ret = string_to_ip(val, pi->ip);
	if (ret)
		return ret;

	if (!pi->set)
		return 0;

	ret = pi->set(p, p->driver_priv);
	if (ret)
		*pi->ip = ip_save;

	return ret;
}

static const char *param_ip_get(struct device_d *dev, struct param_d *p)
{
	struct param_ip *pi = to_param_ip(p);
	int ret;

	if (pi->get) {
		ret = pi->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	free(p->value);
	p->value = xstrdup(ip_to_string(*pi->ip));

	return p->value;
}

struct param_d *dev_add_param_ip(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		IPaddr_t *ip, void *priv)
{
	struct param_ip *pi;
	int ret;

	pi = xzalloc(sizeof(*pi));
	pi->ip = ip;
	pi->set = set;
	pi->get = get;
	pi->param.driver_priv = priv;

	ret = __dev_add_param(&pi->param, dev, name,
			param_ip_set, param_ip_get, 0);
	if (ret) {
		free(pi);
		return ERR_PTR(ret);
	}

	return &pi->param;
}
#endif

/**
 * dev_remove_param - remove a parameter from a device and free its
 * memory
 * @param p	The parameter
 */
void dev_remove_param(struct param_d *p)
{
	p->set(p->dev, p, NULL);
	list_del(&p->list);
	free(p->name);
	free(p);
}

/**
 * dev_remove_parameters - remove all parameters from a device and free their
 * memory
 * @param dev	The device
 */
void dev_remove_parameters(struct device_d *dev)
{
	struct param_d *p, *n;

	list_for_each_entry_safe(p, n, &dev->parameters, list) {
		p->set(dev, p, NULL);
		list_del(&p->list);
		free(p->name);
		free(p);
	}
}

/** @page dev_params Device parameters

@section params_devices Devices can have several parameters.

In case of a network device this may be the IP address, networking mask or
similar and users need access to these parameters. In barebox this is solved
with device paramters. Device parameters are always strings, although there
are functions to interpret them as something else. 'hush' users can access
parameters as a local variable which have a dot (.) in them. So setting the
IP address of the first ethernet device is a matter of typing
'eth0.ip=192.168.0.7' on the console and can then be read back with
'echo $eth0.ip'. The @ref devinfo_command command shows a summary about all
devices currently present. If called with a device id as parameter it shows the
parameters available for a device.

See the individual functions for parameter programming.

*/
