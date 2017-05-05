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
#include <string.h>
#include <globalvar.h>
#include <linux/err.h>

static const char *param_type_string[] = {
	[PARAM_TYPE_STRING] = "string",
	[PARAM_TYPE_INT32] = "int32",
	[PARAM_TYPE_UINT32] = "uint32",
	[PARAM_TYPE_INT64] = "int64",
	[PARAM_TYPE_UINT64] = "uint64",
	[PARAM_TYPE_BOOL] = "bool",
	[PARAM_TYPE_ENUM] = "enum",
	[PARAM_TYPE_BITMASK] = "bitmask",
	[PARAM_TYPE_IPV4] = "ipv4",
	[PARAM_TYPE_MAC] = "MAC",
};

const char *get_param_type(struct param_d *param)
{
	return param_type_string[param->type];
}

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

	return param->get(param);
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

	ret = param->set(param, val);
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
int dev_param_set_generic(struct param_d *p,
		const char *val)
{
	free(p->value);
	if (!val) {
		p->value = NULL;
		return 0;
	}
	p->value = strdup(val);
	return p->value ? 0 : -ENOMEM;
}

static const char *param_get_generic(struct param_d *p)
{
	return p->value ? p->value : "";
}

static int compare(struct list_head *a, struct list_head *b)
{
	char *na = (char*)list_entry(a, struct param_d, list)->name;
	char *nb = (char*)list_entry(b, struct param_d, list)->name;

	return strcmp(na, nb);
}

static int __dev_add_param(struct param_d *param, struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, const char *val),
		const char *(*get)(struct param_d *p),
		unsigned long flags)
{
	if (get_param_by_name(dev, name))
		return -EEXIST;

	param->name = strdup(name);
	if (!param->name)
		return -ENOMEM;

	if (set)
		param->set = set;
	else
		param->set = dev_param_set_generic;
	if (get)
		param->get = get;
	else
		param->get = param_get_generic;

	param->flags = flags;
	param->dev = dev;
	list_add_sort(&param->list, &dev->parameters, compare);

	dev_param_init_from_nv(dev, name);

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
struct param_d *dev_add_param(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, const char *val),
		const char *(*get)(struct param_d *param),
		unsigned long flags)
{
	struct param_d *param;
	int ret;

	param = xzalloc(sizeof(*param));

	ret = __dev_add_param(param, dev, name, set, get, flags);
	if (ret) {
		free(param);
		return ERR_PTR(ret);
	}

	return param;
}

/**
 * dev_add_param_fixed - add a readonly parameter to a device
 * @param dev	The device
 * @param name	The name of the parameter
 * @param value	The value of the parameter
 */
struct param_d *dev_add_param_fixed(struct device_d *dev, const char *name, const char *value)
{
	struct param_d *param;
	int ret;

	param = xzalloc(sizeof(*param));

	ret = __dev_add_param(param, dev, name, NULL, NULL, PARAM_FLAG_RO);
	if (ret) {
		free(param);
		return ERR_PTR(ret);
	}

	param->value = strdup(value);

	return param;
}

struct param_string {
	struct param_d param;
	char **value;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

static inline struct param_string *to_param_string(struct param_d *p)
{
	return container_of(p, struct param_string, param);
}

static int param_string_set(struct param_d *p, const char *val)
{
	struct param_string *ps = to_param_string(p);
	int ret;
	char *value_save = *ps->value;

	if (!val)
		val = "";

	*ps->value = xstrdup(val);

	if (!ps->set)
		return 0;

	ret = ps->set(p, p->driver_priv);
	if (ret) {
		free(*ps->value);
		*ps->value = value_save;
	} else {
		free(value_save);
	}

	return ret;
}

static const char *param_string_get(struct param_d *p)
{
	struct param_string *ps = to_param_string(p);
	int ret;

	if (ps->get) {
		ret = ps->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	return *ps->value;
}

struct param_d *dev_add_param_string(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		char **value, void *priv)
{
	struct param_string *ps;
	struct param_d *p;
	int ret;

	ps = xzalloc(sizeof(*ps));
	ps->value = value;
	ps->set = set;
	ps->get = get;
	p = &ps->param;
	p->driver_priv = priv;
	p->type = PARAM_TYPE_STRING;

	ret = __dev_add_param(p, dev, name, param_string_set, param_string_get, 0);
	if (ret) {
		free(ps);
		return ERR_PTR(ret);
	}

	return &ps->param;
}

struct param_int {
	struct param_d param;
	void *value;
	int dsize;
	const char *format;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

static inline struct param_int *to_param_int(struct param_d *p)
{
	return container_of(p, struct param_int, param);
}

static int param_int_set(struct param_d *p, const char *val)
{
	struct param_int *pi = to_param_int(p);
	u8 value_save[pi->dsize];
	int ret;

	if (!val)
		return -EINVAL;

	memcpy(value_save, pi->value, pi->dsize);

	switch (p->type) {
	case PARAM_TYPE_BOOL:
		ret = strtobool(val, pi->value);
		break;
	case PARAM_TYPE_INT32:
		*(uint32_t *)pi->value = simple_strtol(val, NULL, 0);
		break;
	case PARAM_TYPE_UINT32:
		*(int32_t *)pi->value = simple_strtoul(val, NULL, 0);
		break;
	case PARAM_TYPE_INT64:
		*(int64_t *)pi->value = simple_strtoll(val, NULL, 0);
		break;
	case PARAM_TYPE_UINT64:
		*(uint64_t *)pi->value = simple_strtoull(val, NULL, 0);
		break;
	default:
		return -EINVAL;
	}

	if (!pi->set)
		return 0;

	ret = pi->set(p, p->driver_priv);
	if (ret)
		memcpy(pi->value, value_save, pi->dsize);

	return ret;
}

static const char *param_int_get(struct param_d *p)
{
	struct param_int *pi = to_param_int(p);
	int ret;

	if (pi->get) {
		ret = pi->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	free(p->value);
	switch (p->type) {
	case PARAM_TYPE_BOOL:
	case PARAM_TYPE_INT32:
	case PARAM_TYPE_UINT32:
		p->value = basprintf(pi->format, *(int32_t *)pi->value);
		break;
	case PARAM_TYPE_INT64:
	case PARAM_TYPE_UINT64:
		p->value = basprintf(pi->format, *(int64_t *)pi->value);
		break;
	default:
		return NULL;
	}

	return p->value;
}

int param_set_readonly(struct param_d *p, void *priv)
{
	return -EROFS;
}

/**
 * dev_add_param_int - add an integer parameter to a device
 * @param dev	The device
 * @param name	The name of the parameter
 * @param set	set function
 * @param get	get function
 * @param value	pointer to the integer containing the value of the parameter
 * @param type  The variable type
 * @param format the printf format used to print the value
 * @param priv	user private data, will be passed to get/set
 *
 * The get function can be used as a notifier when the variable is about
 * to be read.
 * The set function can be used as a notifer when the variable is about
 * to be written. Can also be used to limit the value.
 */
struct param_d *__dev_add_param_int(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		void *value, enum param_type type, const char *format, void *priv)
{
	struct param_int *pi;
	struct param_d *p;
	int ret, dsize;

	switch (type) {
	case PARAM_TYPE_BOOL:
		dsize = sizeof(uint32_t);
		break;
	case PARAM_TYPE_INT32:
		dsize = sizeof(int32_t);
		break;
	case PARAM_TYPE_UINT32:
		dsize = sizeof(uint32_t);
		break;
	case PARAM_TYPE_INT64:
		dsize = sizeof(int64_t);
		break;
	case PARAM_TYPE_UINT64:
		dsize = sizeof(uint64_t);
		break;
	default:
		return ERR_PTR(-EINVAL);
	}

	pi = xzalloc(sizeof(*pi));

	if (IS_ERR(set)) {
		pi->value = xmemdup(value, dsize);
		set = param_set_readonly;
	} else {
		pi->value = value;
	}

	pi->dsize = dsize;
	pi->format = format;
	pi->set = set;
	pi->get = get;
	p = &pi->param;
	p->driver_priv = priv;
	p->type = type;

	ret = __dev_add_param(p, dev, name, param_int_set, param_int_get, 0);
	if (ret) {
		free(pi);
		return ERR_PTR(ret);
	}

	return &pi->param;
}

struct param_enum {
	struct param_d param;
	unsigned int *value;
	const char * const *names;
	int num_names;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

static inline struct param_enum *to_param_enum(struct param_d *p)
{
	return container_of(p, struct param_enum, param);
}

static int param_enum_set(struct param_d *p, const char *val)
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

static const char *param_enum_get(struct param_d *p)
{
	struct param_enum *pe = to_param_enum(p);
	int ret;

	if (pe->get) {
		ret = pe->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	free(p->value);

	if (*pe->value >= pe->num_names)
		p->value = basprintf("invalid:%d", *pe->value);
	else
		p->value = strdup(pe->names[*pe->value]);

	return p->value;
}

static void param_enum_info(struct param_d *p)
{
	struct param_enum *pe = to_param_enum(p);
	int i;

	if (pe->num_names <= 1)
		return;

	printf(" (values: ");

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
		int *value, const char * const *names, int num_names, void *priv)
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
	p->type = PARAM_TYPE_ENUM;

	ret = __dev_add_param(p, dev, name, param_enum_set, param_enum_get, 0);
	if (ret) {
		free(pe);
		return ERR_PTR(ret);
	}

	p->info = param_enum_info;

	return &pe->param;
}

struct param_bitmask {
	struct param_d param;
	unsigned long *value;
	const char * const *names;
	int num_names;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

static inline struct param_bitmask *to_param_bitmask(struct param_d *p)
{
	return container_of(p, struct param_bitmask, param);
}

static int param_bitmask_set(struct param_d *p, const char *val)
{
	struct param_bitmask *pb = to_param_bitmask(p);
	void *value_save;
	int i, ret;
	char *freep, *dval, *str;

	if (!val)
		val = "";

	freep = dval = xstrdup(val);
	value_save = xmemdup(pb->value, BITS_TO_LONGS(pb->num_names) * sizeof(unsigned long));

	while (1) {
		str = strsep(&dval, " ");
		if (!str || !*str)
			break;

		for (i = 0; i < pb->num_names; i++) {
			if (pb->names[i] && !strcmp(str, pb->names[i])) {
				set_bit(i, pb->value);
				break;
			}
		}

		if (i == pb->num_names) {
			ret = -EINVAL;
			goto out;
		}
	}

	if (!pb->set) {
		ret = 0;
		goto out;
	}

	ret = pb->set(p, p->driver_priv);
	if (ret)
		memcpy(pb->value, value_save, BITS_TO_LONGS(pb->num_names) * sizeof(unsigned long));

out:
	free(value_save);
	free(freep);
	return ret;
}

static const char *param_bitmask_get(struct param_d *p)
{
	struct param_bitmask *pb = to_param_bitmask(p);
	int ret, bit;
	char *pos;

	if (pb->get) {
		ret = pb->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	pos = p->value;

	for_each_set_bit(bit, pb->value, pb->num_names)
		if (pb->names[bit])
			pos += sprintf(pos, "%s ", pb->names[bit]);

	return p->value;
}

static void param_bitmask_info(struct param_d *p)
{
	struct param_bitmask *pb = to_param_bitmask(p);
	int i;

	if (pb->num_names <= 1)
		return;

	printf(" (list: ");

	for (i = 0; i < pb->num_names; i++) {
		if (!pb->names[i] || !*pb->names[i])
			continue;
		printf("\"%s\"%s", pb->names[i],
				i ==  pb->num_names - 1 ? ")" : ", ");
	}
}

struct param_d *dev_add_param_bitmask(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		unsigned long *value, const char * const *names, int max, void *priv)
{
	struct param_bitmask *pb;
	struct param_d *p;
	int ret, i, len = 0;

	pb = xzalloc(sizeof(*pb));

	pb->value = value;
	pb->set = set;
	pb->get = get;
	pb->names = names;
	pb->num_names = max;
	p = &pb->param;
	p->driver_priv = priv;
	p->type = PARAM_TYPE_BITMASK;

	for (i = 0; i < pb->num_names; i++)
		if (pb->names[i])
			len += strlen(pb->names[i]) + 1;

	p->value = xzalloc(len);

	ret = __dev_add_param(p, dev, name, param_bitmask_set, param_bitmask_get, 0);
	if (ret) {
		free(pb);
		return ERR_PTR(ret);
	}

	p->info = param_bitmask_info;

	return &pb->param;
}

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

static int param_ip_set(struct param_d *p, const char *val)
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

static const char *param_ip_get(struct param_d *p)
{
	struct param_ip *pi = to_param_ip(p);
	int ret;

	if (pi->get) {
		ret = pi->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	free(p->value);
	p->value = xasprintf("%pI4", pi->ip);

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
	pi->param.type = PARAM_TYPE_IPV4;

	ret = __dev_add_param(&pi->param, dev, name,
			param_ip_set, param_ip_get, 0);
	if (ret) {
		free(pi);
		return ERR_PTR(ret);
	}

	return &pi->param;
}

struct param_mac {
	struct param_d param;
	char *mac;
	u8 mac_str[sizeof("xx:xx:xx:xx:xx:xx")];
	const char *format;
	int (*set)(struct param_d *p, void *priv);
	int (*get)(struct param_d *p, void *priv);
};

int string_to_ethaddr(const char *str, u8 enetaddr[6]);
void ethaddr_to_string(const u8 enetaddr[6], char *str);

static inline struct param_mac *to_param_mac(struct param_d *p)
{
	return container_of(p, struct param_mac, param);
}

static int param_mac_set(struct param_d *p, const char *val)
{
	struct param_mac *pm = to_param_mac(p);
	char mac_save[6];
	int ret;

	if (!val)
		return -EINVAL;

	memcpy(mac_save, pm->mac, 6);

	ret = string_to_ethaddr(val, pm->mac);
	if (ret)
		goto out;

	if (!pm->set)
		return 0;

	ret = pm->set(p, p->driver_priv);
	if (ret)
		goto out;

	return 0;
out:
	memcpy(pm->mac, mac_save, 6);

	return ret;
}

static const char *param_mac_get(struct param_d *p)
{
	struct param_mac *pm = to_param_mac(p);
	int ret;

	if (pm->get) {
		ret = pm->get(p, p->driver_priv);
		if (ret)
			return NULL;
	}

	ethaddr_to_string(pm->mac, p->value);

	return p->value;
}

struct param_d *dev_add_param_mac(struct device_d *dev, const char *name,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		u8 *mac, void *priv)
{
	struct param_mac *pm;
	int ret;

	pm = xzalloc(sizeof(*pm));
	pm->mac = mac;
	pm->set = set;
	pm->get = get;
	pm->param.driver_priv = priv;
	pm->param.value = pm->mac_str;
	pm->param.type = PARAM_TYPE_MAC;

	ret = __dev_add_param(&pm->param, dev, name,
			param_mac_set, param_mac_get, 0);
	if (ret) {
		free(pm);
		return ERR_PTR(ret);
	}

	return &pm->param;
}

/**
 * dev_remove_param - remove a parameter from a device and free its
 * memory
 * @param p	The parameter
 */
void dev_remove_param(struct param_d *p)
{
	p->set(p, NULL);
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
		p->set(p, NULL);
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
'eth0.ipaddr=192.168.0.7' on the console and can then be read back with
'echo $eth0.ipaddr'. The @ref devinfo_command command shows a summary about all
devices currently present. If called with a device id as parameter it shows the
parameters available for a device.

See the individual functions for parameter programming.

*/
