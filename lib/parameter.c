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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
		errno = -EINVAL;
		return NULL;
	}

	return param->get(dev, param);
}

#ifdef CONFIG_NET
IPaddr_t dev_get_param_ip(struct device_d *dev, char *name)
{
	IPaddr_t ip;

	if (string_to_ip(dev_get_param(dev, name), &ip))
		return 0;

	return ip;
}

int dev_set_param_ip(struct device_d *dev, char *name, IPaddr_t ip)
{
	char ipstr[sizeof("xxx.xxx.xxx.xxx")];

	ip_to_string(ip, ipstr);

	return dev_set_param(dev, name, ipstr);
}
#endif

/**
 * dev_set_param - set a parameter of a device to a new value
 * @param dev	The device
 * @param name	The name of the parameter
 * @param val	The new value of the parameter
 */
int dev_set_param(struct device_d *dev, const char *name, const char *val)
{
	struct param_d *param;

	if (!dev) {
		errno = -ENODEV;
		return -ENODEV;
	}

	param = get_param_by_name(dev, name);

	if (!param) {
		errno = -EINVAL;
		return -EINVAL;
	}

	if (param->flags & PARAM_FLAG_RO) {
		errno = -EACCES;
		return -EACCES;
	}

	errno = param->set(dev, param, val);
	return errno;
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

static char *param_get_generic(struct device_d *dev, struct param_d *p)
{
	return p->value;
}

static struct param_d *__dev_add_param(struct device_d *dev, char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		char *(*get)(struct device_d *dev, struct param_d *p),
		unsigned long flags)
{
	struct param_d *param;

	param = xzalloc(sizeof(*param));

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
	list_add_tail(&param->list, &dev->parameters);

	return param;
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
int dev_add_param(struct device_d *dev, char *name,
		int (*set)(struct device_d *dev, struct param_d *p, const char *val),
		char *(*get)(struct device_d *dev, struct param_d *param),
		unsigned long flags)
{
	struct param_d *param;

	param = __dev_add_param(dev, name, set, get, flags);

	return param ? 0 : -EINVAL;
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

	param = __dev_add_param(dev, name, NULL, NULL, PARAM_FLAG_RO);
	if (!param)
		return -EINVAL;

	param->value = strdup(value);

	return 0;
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
