/*
 * Copyright (C) 2012-2014 Pengutronix, Jan Luebbe <j.luebbe@pengutronix.de>
 * Copyright (C) 2013-2014 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * Copyright (C) 2015 Pengutronix, Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/types.h>
#include <malloc.h>
#include <net.h>
#include <printk.h>
#include <of.h>
#include <stdio.h>

#include "state.h"

/**
 * state_set_dirty - Helper function to set the state to dirty. Only used for
 * state variables callbacks
 * @param p
 * @param priv
 * @return
 */
static int state_set_dirty(struct param_d *p, void *priv)
{
	struct state_variable *sv = priv;

	sv->state->dirty = 1;

	return 0;
}

static int state_var_compare(struct list_head *a, struct list_head *b)
{
	struct state_variable *va = list_entry(a, struct state_variable, list);
	struct state_variable *vb = list_entry(b, struct state_variable, list);

	return va->start < vb->start ? -1 : 1;
}

void state_add_var(struct state *state, struct state_variable *var)
{
	list_add_sort(&var->list, &state->variables, state_var_compare);
}

static int state_uint32_export(struct state_variable *var,
			       struct device_node *node,
			       enum state_convert conv)
{
	struct state_uint32 *su32 = to_state_uint32(var);
	int ret;

	if (su32->value_default) {
		ret = of_property_write_u32(node, "default",
					    su32->value_default);
		if (ret)
			return ret;
	}

	if (conv == STATE_CONVERT_FIXUP)
		return 0;

	return of_property_write_u32(node, "value", su32->value);
}

static int state_uint32_import(struct state_variable *sv,
			       struct device_node *node)
{
	struct state_uint32 *su32 = to_state_uint32(sv);

	of_property_read_u32(node, "default", &su32->value_default);
	if (of_property_read_u32(node, "value", &su32->value))
		su32->value = su32->value_default;

	return 0;
}

static int state_uint8_set(struct param_d *p, void *priv)
{
	struct state_variable *sv = priv;
	struct state_uint32 *su32 = to_state_uint32(sv);

	if (su32->value > 255)
		return -ERANGE;

	return state_set_dirty(p, sv);
}

static struct state_variable *state_uint8_create(struct state *state,
						 const char *name,
						 struct device_node *node)
{
	struct state_uint32 *su32;
	struct param_d *param;

	su32 = xzalloc(sizeof(*su32));

	param = dev_add_param_int(&state->dev, name, state_uint8_set,
				  NULL, &su32->value, "%u", &su32->var);
	if (IS_ERR(param)) {
		free(su32);
		return ERR_CAST(param);
	}

	su32->param = param;
	su32->var.size = sizeof(uint8_t);
#ifdef __LITTLE_ENDIAN
	su32->var.raw = &su32->value;
#else
	su32->var.raw = &su32->value + 3;
#endif
	su32->var.state = state;

	return &su32->var;
}

static struct state_variable *state_uint32_create(struct state *state,
						  const char *name,
						  struct device_node *node)
{
	struct state_uint32 *su32;
	struct param_d *param;

	su32 = xzalloc(sizeof(*su32));

	param = dev_add_param_int(&state->dev, name, state_set_dirty,
				  NULL, &su32->value, "%u", &su32->var);
	if (IS_ERR(param)) {
		free(su32);
		return ERR_CAST(param);
	}

	su32->param = param;
	su32->var.size = sizeof(uint32_t);
	su32->var.raw = &su32->value;
	su32->var.state = state;

	return &su32->var;
}

static int state_enum32_export(struct state_variable *var,
			       struct device_node *node,
			       enum state_convert conv)
{
	struct state_enum32 *enum32 = to_state_enum32(var);
	int ret, i, len;
	char *prop, *str;

	if (enum32->value_default) {
		ret = of_property_write_u32(node, "default",
					    enum32->value_default);
		if (ret)
			return ret;
	}

	len = 0;

	for (i = 0; i < enum32->num_names; i++)
		len += strlen(enum32->names[i]) + 1;

	prop = xzalloc(len);
	str = prop;

	for (i = 0; i < enum32->num_names; i++)
		str += sprintf(str, "%s", enum32->names[i]) + 1;

	ret = of_set_property(node, "names", prop, len, 1);

	free(prop);

	if (conv == STATE_CONVERT_FIXUP)
		return 0;

	ret = of_property_write_u32(node, "value", enum32->value);
	if (ret)
		return ret;

	return ret;
}

static int state_enum32_import(struct state_variable *sv,
			       struct device_node *node)
{
	struct state_enum32 *enum32 = to_state_enum32(sv);
	int len;
	const __be32 *value, *value_default;

	value = of_get_property(node, "value", &len);
	if (value && len != sizeof(uint32_t))
		return -EINVAL;

	value_default = of_get_property(node, "default", &len);
	if (value_default && len != sizeof(uint32_t))
		return -EINVAL;

	if (value_default)
		enum32->value_default = be32_to_cpu(*value_default);
	if (value)
		enum32->value = be32_to_cpu(*value);
	else
		enum32->value = enum32->value_default;

	return 0;
}

static struct state_variable *state_enum32_create(struct state *state,
						  const char *name,
						  struct device_node *node)
{
	struct state_enum32 *enum32;
	int ret, i, num_names;

	enum32 = xzalloc(sizeof(*enum32));

	num_names = of_property_count_strings(node, "names");
	if (num_names < 0) {
		dev_err(&state->dev,
			"enum32 node without \"names\" property\n");
		return ERR_PTR(-EINVAL);
	}

	enum32->names = xzalloc(sizeof(char *) * num_names);
	enum32->num_names = num_names;
	enum32->var.size = sizeof(uint32_t);
	enum32->var.raw = &enum32->value;
	enum32->var.state = state;

	for (i = 0; i < num_names; i++) {
		const char *name;

		ret = of_property_read_string_index(node, "names", i, &name);
		if (ret)
			goto out;
		enum32->names[i] = xstrdup(name);
	}

	enum32->param = dev_add_param_enum(&state->dev, name, state_set_dirty,
					   NULL, &enum32->value, enum32->names,
					   num_names, &enum32->var);
	if (IS_ERR(enum32->param)) {
		ret = PTR_ERR(enum32->param);
		goto out;
	}

	return &enum32->var;
 out:	for (i--; i >= 0; i--)
		free((char *)enum32->names[i]);
	free(enum32->names);
	free(enum32);
	return ERR_PTR(ret);
}

static int state_mac_export(struct state_variable *var,
			    struct device_node *node, enum state_convert conv)
{
	struct state_mac *mac = to_state_mac(var);
	int ret;

	if (!is_zero_ether_addr(mac->value_default)) {
		ret = of_property_write_u8_array(node, "default",
						 mac->value_default,
						 ARRAY_SIZE(mac->
							    value_default));
		if (ret)
			return ret;
	}

	if (conv == STATE_CONVERT_FIXUP)
		return 0;

	return of_property_write_u8_array(node, "value", mac->value,
					  ARRAY_SIZE(mac->value));
}

static int state_mac_import(struct state_variable *sv, struct device_node *node)
{
	struct state_mac *mac = to_state_mac(sv);

	of_property_read_u8_array(node, "default", mac->value_default,
				  ARRAY_SIZE(mac->value_default));
	if (of_property_read_u8_array(node, "value", mac->value,
				      ARRAY_SIZE(mac->value)))
		memcpy(mac->value, mac->value_default, ARRAY_SIZE(mac->value));

	return 0;
}

static struct state_variable *state_mac_create(struct state *state,
					       const char *name,
					       struct device_node *node)
{
	struct state_mac *mac;
	int ret;

	mac = xzalloc(sizeof(*mac));

	mac->var.size = ARRAY_SIZE(mac->value);
	mac->var.raw = mac->value;
	mac->var.state = state;

	mac->param = dev_add_param_mac(&state->dev, name, state_set_dirty,
				       NULL, mac->value, &mac->var);
	if (IS_ERR(mac->param)) {
		ret = PTR_ERR(mac->param);
		goto out;
	}

	return &mac->var;
 out:	free(mac);
	return ERR_PTR(ret);
}

static int state_string_export(struct state_variable *var,
			       struct device_node *node,
			       enum state_convert conv)
{
	struct state_string *string = to_state_string(var);
	int ret = 0;

	if (string->value_default) {
		ret = of_set_property(node, "default", string->value_default,
				      strlen(string->value_default) + 1, 1);

		if (ret)
			return ret;
	}

	if (conv == STATE_CONVERT_FIXUP)
		return 0;

	if (string->value)
		ret = of_set_property(node, "value", string->value,
				      strlen(string->value) + 1, 1);

	return ret;
}

static int state_string_import(struct state_variable *sv,
			       struct device_node *node)
{
	struct state_string *string = to_state_string(sv);
	const char *value = NULL;
	size_t len;
	int ret;

	of_property_read_string(node, "default", &string->value_default);
	if (string->value_default) {
		len = strlen(string->value_default);
		if (len > string->var.size)
			return -EILSEQ;
	}

	ret = of_property_read_string(node, "value", &value);
	if (ret)
		value = string->value_default;

	if (value)
		return state_string_copy_to_raw(string, value);

	return 0;
}

static int state_string_set(struct param_d *p, void *priv)
{
	struct state_variable *sv = priv;
	struct state_string *string = to_state_string(sv);
	int ret;

	ret = state_string_copy_to_raw(string, string->value);
	if (ret)
		return ret;

	return state_set_dirty(p, sv);
}

static int state_string_get(struct param_d *p, void *priv)
{
	struct state_variable *sv = priv;
	struct state_string *string = to_state_string(sv);

	free(string->value);
	if (string->raw[0])
		string->value = xstrndup(string->raw, string->var.size);
	else
		string->value = xstrdup("");

	return 0;
}

static struct state_variable *state_string_create(struct state *state,
						  const char *name,
						  struct device_node *node)
{
	struct state_string *string;
	uint32_t start_size[2];
	int ret;

	ret = of_property_read_u32_array(node, "reg", start_size,
					 ARRAY_SIZE(start_size));
	if (ret) {
		dev_err(&state->dev, "%s: reg property not found\n", name);
		return ERR_PTR(ret);
	}

	/* limit to arbitrary len of 4k */
	if (start_size[1] > 4096)
		return ERR_PTR(-EILSEQ);

	string = xzalloc(sizeof(*string) + start_size[1]);
	string->var.size = start_size[1];
	string->var.raw = &string->raw;
	string->var.state = state;

	string->param = dev_add_param_string(&state->dev, name,
					     state_string_set, state_string_get,
					     &string->value, &string->var);
	if (IS_ERR(string->param)) {
		ret = PTR_ERR(string->param);
		goto out;
	}

	return &string->var;
 out:	free(string);
	return ERR_PTR(ret);
}

static struct variable_type types[] = {
	{
		.type = STATE_TYPE_U8,
		.type_name = "uint8",
		.export = state_uint32_export,
		.import = state_uint32_import,
		.create = state_uint8_create,
	}, {
		.type = STATE_TYPE_U32,
		.type_name = "uint32",
		.export = state_uint32_export,
		.import = state_uint32_import,
		.create = state_uint32_create,
	}, {
		.type = STATE_TYPE_ENUM,
		.type_name = "enum32",
		.export = state_enum32_export,
		.import = state_enum32_import,
		.create = state_enum32_create,
	}, {
		.type = STATE_TYPE_MAC,
		.type_name = "mac",
		.export = state_mac_export,
		.import = state_mac_import,
		.create = state_mac_create,
	}, {
		.type = STATE_TYPE_STRING,
		.type_name = "string",
		.export = state_string_export,
		.import = state_string_import,
		.create = state_string_create,
	}
};

struct variable_type *state_find_type_by_name(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		if (!strcmp(name, types[i].type_name)) {
			return &types[i];
		}
	}

	return NULL;
}

struct state_variable *state_find_var(struct state *state, const char *name)
{
	struct state_variable *sv;

	list_for_each_entry(sv, &state->variables, list) {
		if (!strcmp(sv->name, name))
			return sv;
	}

	return ERR_PTR(-ENOENT);
}
