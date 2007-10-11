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
	return string_to_ip(dev_get_param(dev, name));
}

int dev_set_param_ip(struct device_d *dev, char *name, IPaddr_t ip)
{
	char ipstr[16];
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

	if (param->set)
		return param->set(dev, param, val);

	if (param->value)
		free(param->value);

	param->value = strdup(val);
	return 0;
}

int dev_add_param(struct device_d *dev, struct param_d *newparam)
{
	struct param_d *param = dev->param;

	newparam->next = 0;

	if (param) {
		while (param->next)
			param = param->next;
			param->next = newparam;
	} else {
		dev->param = newparam;
	}

	return 0;
}
