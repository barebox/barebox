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
	struct param_d *param = dev->param;

	while (param) {
		if (!strcmp(param->name, name))
			return param;
		param = param->next;
	}

	return NULL;
}

const char *dev_get_param(struct device_d *dev, const char *name)
{
	struct param_d *param = get_param_by_name(dev, name);

	if (!param) {
		errno = -EINVAL;
		return NULL;
	}

	if (param->get)
		return param->get(dev, param);

	return param->value;
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

	if (param->set) {
		errno = param->set(dev, param, val);
		return errno;
	}

	if (param->value)
		free(param->value);

	param->value = strdup(val);
	return 0;
}

int dev_add_param(struct device_d *dev, struct param_d *newparam)
{
	struct param_d *param = dev->param;

	newparam->next = NULL;

	if (param) {
		while (param->next)
			param = param->next;
			param->next = newparam;
	} else {
		dev->param = newparam;
	}

	return 0;
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

@section params_programming Device parameters programming API

@code
struct param_d {
	char* (*get)(struct device_d *, struct param_d *param);
	int (*set)(struct device_d *, struct param_d *param, const char *val);
	ulong flags;
	char *name;
	struct param_d *next;
	char *value;
};
@endcode

@code
int dev_add_param(struct device_d *dev, struct param_d *newparam);
@endcode

This function adds a new parameter to a device. At least the name field in
the new parameter struct has to be initialized. The 'get' and 'set' fields
can be set to NULL in which case the framework handles them. It is also
allowed to implement only one of the get/set functions. Care must be taken
with the initial value of the parameter. If the framework handles the set
function it will try to free the value of the parameter. If this is a
static array bad things will happen. A parameter can have the flag
PARAM_FLAG_RO which means that the parameter is readonly. It is perfectly ok
then to point the value to a static array.

@code
const char *dev_get_param(struct device_d *dev, const char *name);
@endcode

This function returns a pointer to the value of the parameter specified
with dev and name.
If the framework handles the get/set functions the parameter value strings
are alloceted with malloc and freed with free when another value is set for
this parameter. Drivers implementing set/get themselves are allowed to
return values in static arrays. This means that the pointers returned from
dev_get_param() are only valid until the next call to dev_get_param. If this
is not long enough strdup() or similar must be used.

@code
int dev_set_param(struct device_d *dev, const char *name, const char *val);
@endcode

Set the value of a parameter.

*/
