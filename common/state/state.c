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

#include <asm-generic/ioctl.h>
#include <common.h>
#include <digest.h>
#include <errno.h>
#include <fs.h>
#include <crc.h>
#include <init.h>
#include <linux/err.h>
#include <linux/list.h>

#include <linux/mtd/mtd-abi.h>
#include <malloc.h>
#include <state.h>
#include <libbb.h>

#include "state.h"

/* list of all registered state instances */
static LIST_HEAD(state_list);

static struct state *state_new(const char *name)
{
	struct state *state;
	int ret;

	state = xzalloc(sizeof(*state));
	safe_strncpy(state->dev.name, name, MAX_DRIVER_NAME);
	state->name = state->dev.name;
	state->dev.id = DEVICE_ID_SINGLE;
	INIT_LIST_HEAD(&state->variables);

	ret = register_device(&state->dev);
	if (ret) {
		pr_err("Failed to register state device %s, %d\n", name, ret);
		free(state);
		return ERR_PTR(ret);
	}

	state->dirty = 1;
	dev_add_param_bool(&state->dev, "dirty", NULL, NULL, &state->dirty,
			   NULL);
	state->save_on_shutdown = 1;
	dev_add_param_bool(&state->dev, "save_on_shutdown", NULL, NULL,
			   &state->save_on_shutdown, NULL);

	list_add_tail(&state->list, &state_list);

	return state;
}

static int state_convert_node_variable(struct state *state,
				       struct device_node *node,
				       struct device_node *parent,
				       const char *parent_name,
				       enum state_convert conv)
{
	const struct variable_type *vtype;
	struct device_node *child;
	struct device_node *new_node = NULL;
	struct state_variable *sv;
	const char *type_name;
	char *short_name, *name, *indexs;
	unsigned int start_size[2];
	int ret;

	/* strip trailing @<ADDRESS> */
	short_name = xstrdup(node->name);
	indexs = strchr(short_name, '@');
	if (indexs)
		*indexs = 0;

	/* construct full name */
	name = basprintf("%s%s%s", parent_name, parent_name[0] ? "." : "",
			 short_name);
	free(short_name);

	if ((conv == STATE_CONVERT_TO_NODE) || (conv == STATE_CONVERT_FIXUP))
		new_node = of_new_node(parent, node->name);

	for_each_child_of_node(node, child) {
		ret = state_convert_node_variable(state, child, new_node, name,
						  conv);
		if (ret)
			goto out_free;
	}

	/* parents are allowed to have no type */
	ret = of_property_read_string(node, "type", &type_name);
	if (!list_empty(&node->children) && ret == -EINVAL) {
		if (conv == STATE_CONVERT_FIXUP) {
			ret = of_property_write_u32(new_node, "#address-cells",
						    1);
			if (ret)
				goto out_free;

			ret = of_property_write_u32(new_node, "#size-cells", 1);
			if (ret)
				goto out_free;
		}
		ret = 0;
		goto out_free;
	} else if (ret) {
		goto out_free;
	}

	vtype = state_find_type_by_name(type_name);
	if (!vtype) {
		ret = -ENOENT;
		goto out_free;
	}

	if (conv == STATE_CONVERT_FROM_NODE_CREATE) {
		sv = vtype->create(state, name, node);
		if (IS_ERR(sv)) {
			ret = PTR_ERR(sv);
			dev_err(&state->dev, "failed to create %s: %s\n", name,
				strerror(-ret));
			goto out_free;
		}

		ret = of_property_read_u32_array(node, "reg", start_size,
						 ARRAY_SIZE(start_size));
		if (ret) {
			dev_err(&state->dev, "%s: reg property not found\n",
				name);
			goto out_free;
		}

		if (start_size[1] != sv->size) {
			dev_err(&state->dev,
				"%s: size mismatch: type=%s(size=%u) size=%u\n",
				name, type_name, sv->size, start_size[1]);
			ret = -EOVERFLOW;
			goto out_free;
		}

		sv->name = name;
		sv->start = start_size[0];
		sv->type = vtype->type;
		state_add_var(state, sv);
	} else {
		sv = state_find_var(state, name);
		if (IS_ERR(sv)) {
			/* we ignore this error */
			dev_dbg(&state->dev, "no such variable: %s: %s\n", name,
				strerror(-ret));
			ret = 0;
			goto out_free;
		}
		free(name);

		if ((conv == STATE_CONVERT_TO_NODE)
		    || (conv == STATE_CONVERT_FIXUP)) {
			ret = of_set_property(new_node, "type",
					      vtype->type_name,
					      strlen(vtype->type_name) + 1, 1);
			if (ret)
				goto out;

			start_size[0] = sv->start;
			start_size[1] = sv->size;
			ret = of_property_write_u32_array(new_node, "reg",
							  start_size,
							  ARRAY_SIZE
							  (start_size));
			if (ret)
				goto out;
		}
	}

	if ((conv == STATE_CONVERT_TO_NODE) || (conv == STATE_CONVERT_FIXUP))
		ret = vtype->export(sv, new_node, conv);
	else
		ret = vtype->import(sv, node);

	if (ret)
		goto out;

	return 0;
 out_free:free(name);
 out:	return ret;
}

struct device_node *state_to_node(struct state *state,
				  struct device_node *parent,
				  enum state_convert conv)
{
	struct device_node *child;
	struct device_node *root, *state_root;
	int ret;

	state_root = of_find_node_by_path(state->of_path);
	if (!state_root)
		return ERR_PTR(-ENODEV);

	root = of_new_node(parent, state_root->name);
	ret = of_property_write_u32(root, "magic", state->magic);
	if (ret)
		goto out;

	for_each_child_of_node(state_root, child) {
		ret = state_convert_node_variable(state, child, root, "", conv);
		if (ret)
			goto out;
	}

	return root;
 out:	of_delete_node(root);
	return ERR_PTR(ret);
}

int state_from_node(struct state *state, struct device_node *node, bool create)
{
	struct device_node *child;
	enum state_convert conv;
	int ret;
	uint32_t magic;

	ret = of_property_read_u32(node, "magic", &magic);
	if (ret)
		return ret;

	if (create) {
		conv = STATE_CONVERT_FROM_NODE_CREATE;
		state->of_path = xstrdup(node->full_name);
		state->magic = magic;
	} else {
		conv = STATE_CONVERT_FROM_NODE;
		if (state->magic && state->magic != magic) {
			dev_err(&state->dev,
				"invalid magic 0x%08x, should be 0x%08x\n",
				magic, state->magic);
			return -EINVAL;
		}
	}

	for_each_child_of_node(node, child) {
		ret = state_convert_node_variable(state, child, NULL, "", conv);
		if (ret)
			return ret;
	}

	/* check for overlapping variables */
	if (create) {
		const struct state_variable *sv;

		/* start with second entry */
		sv = list_first_entry(&state->variables, struct state_variable,
				      list);

		list_for_each_entry_continue(sv, &state->variables, list) {
			const struct state_variable *last_sv;

			last_sv = list_last_entry(&sv->list,
						  struct state_variable, list);
			if ((last_sv->start + last_sv->size - 1) < sv->start)
				continue;

			dev_err(&state->dev,
				"ERROR: Conflicting variable position between: "
				"%s (0x%02x..0x%02x) and %s (0x%02x..0x%02x)\n",
				last_sv->name, last_sv->start,
				last_sv->start + last_sv->size - 1,
				sv->name, sv->start, sv->start + sv->size - 1);

			ret |= -EINVAL;
		}
	}

	return ret;
}

static int of_state_fixup(struct device_node *root, void *ctx)
{
	struct state *state = ctx;
	const char *compatible = "barebox,state";
	struct device_node *new_node, *node, *parent, *backend_node;
	struct property *p;
	int ret;
	phandle phandle;

	node = of_find_node_by_path_from(root, state->of_path);
	if (node) {
		/* replace existing node - it will be deleted later */
		parent = node->parent;
	} else {
		char *of_path, *c;

		/* look for parent, remove last '/' from path */
		of_path = xstrdup(state->of_path);
		c = strrchr(of_path, '/');
		if (!c)
			return -ENODEV;
		*c = '\0';
		parent = of_find_node_by_path_from(root, of_path);
		if (!parent)
			parent = root;

		free(of_path);
	}

	/* serialize variable definitions */
	new_node = state_to_node(state, parent, STATE_CONVERT_FIXUP);
	if (IS_ERR(new_node))
		return PTR_ERR(new_node);

	/* compatible */
	p = of_new_property(new_node, "compatible", compatible,
			    strlen(compatible) + 1);
	if (!p) {
		ret = -ENOMEM;
		goto out;
	}

	/* backend-type */
	if (!state->backend.format) {
		ret = -ENODEV;
		goto out;
	}

	p = of_new_property(new_node, "backend-type",
			    state->backend.format->name,
			    strlen(state->backend.format->name) + 1);
	if (!p) {
		ret = -ENOMEM;
		goto out;
	}

	/* backend phandle */
	backend_node = of_find_node_by_path_from(root, state->backend.of_path);
	if (!backend_node) {
		ret = -ENODEV;
		goto out;
	}

	phandle = of_node_create_phandle(backend_node);
	ret = of_property_write_u32(new_node, "backend", phandle);
	if (ret)
		goto out;

	if (!strcmp("raw", state->backend.format->name)) {
		struct digest *digest =
		    state_backend_format_raw_get_digest(state->backend.format);
		if (digest) {
			p = of_new_property(new_node, "algo",
					    digest_name(digest),
					    strlen(digest_name(digest)) + 1);
			if (!p) {
				ret = -ENOMEM;
				goto out;
			}
		}
	}

	if (state->backend.storage.name) {
		p = of_new_property(new_node, "backend-storage-type",
				    state->backend.storage.name,
				    strlen(state->backend.storage.name) + 1);
		if (!p) {
			ret = -ENOMEM;
			goto out;
		}
	}

	if (state->backend.storage.stridesize) {
		ret = of_property_write_u32(new_node, "backend-stridesize",
				state->backend.storage.stridesize);
		if (ret)
			goto out;
	}

	/* address-cells + size-cells */
	ret = of_property_write_u32(new_node, "#address-cells", 1);
	if (ret)
		goto out;

	ret = of_property_write_u32(new_node, "#size-cells", 1);
	if (ret)
		goto out;

	/* delete existing node */
	if (node)
		of_delete_node(node);

	return 0;

 out:	of_delete_node(new_node);
	return ret;
}

void state_release(struct state *state)
{
	of_unregister_fixup(of_state_fixup, state);
	list_del(&state->list);
	unregister_device(&state->dev);
	state_backend_free(&state->backend);
	free(state->of_path);
	free(state);
}

/*
 * state_new_from_node - create a new state instance from a device_node
 *
 * @node	The device_node describing the new state instance
 * @path	Path to the backend device. If NULL the path is constructed
 *		using the path in the backend property of the DT.
 * @offset	Offset in the device path. May be 0 to start at the beginning.
 * @max_size	Maximum size of the area used. This may be 0 to use the full
 *		size.
 * @readonly	This is a read-only state. Note that with this option set,
 *		there are no repairs done.
 */
struct state *state_new_from_node(struct device_node *node, char *path,
				  off_t offset, size_t max_size, bool readonly)
{
	struct state *state;
	int ret = 0;
	int len;
	const char *backend_type;
	const char *storage_type;
	const char *of_path;
	const char *alias;
	uint32_t stridesize;

	alias = of_alias_get(node);
	if (!alias)
		alias = node->name;

	state = state_new(alias);
	if (IS_ERR(state))
		return state;

	of_path = of_get_property(node, "backend", &len);
	if (!of_path) {
		ret = -ENODEV;
		goto out_release_state;
	}

	if (!path) {
		/* guess if of_path is a path, not a phandle */
		if (of_path[0] == '/' && len > 1) {
			ret = of_find_path(node, "backend", &path, 0);
		} else {
			struct device_node *partition_node;

			partition_node = of_parse_phandle(node, "backend", 0);
			if (!partition_node)
				goto out_release_state;

			of_path = partition_node->full_name;
			ret = of_find_path_by_node(partition_node, &path, 0);
		}
		if (!path) {
			pr_err("state failed to parse path to backend\n");
			ret = -EINVAL;
			goto out_release_state;
		}
	}

	ret = of_property_read_string(node, "backend-type", &backend_type);
	if (ret) {
		goto out_release_state;
	}

	ret = of_property_read_u32(node, "backend-stridesize", &stridesize);
	if (ret) {
		stridesize = 0;
	}

	ret = of_property_read_string(node, "backend-storage-type",
				      &storage_type);
	if (ret) {
		storage_type = NULL;
		pr_info("No backend-storage-type found, using default.\n");
	}

	ret = state_backend_init(&state->backend, &state->dev, node,
				 backend_type, path, alias, of_path, offset,
				 max_size, stridesize, storage_type);
	if (ret)
		goto out_release_state;

	if (readonly)
		state_backend_set_readonly(&state->backend);

	ret = state_from_node(state, node, 1);
	if (ret) {
		goto out_release_state;
	}

	ret = of_register_fixup(of_state_fixup, state);
	if (ret) {
		goto out_release_state;
	}

	ret = state_load(state);
	if (ret) {
		pr_warn("Failed to load persistent state, continuing with defaults, %d\n", ret);
	}

	pr_info("New state registered '%s'\n", alias);

	return state;

out_release_state:
	state_release(state);
	return ERR_PTR(ret);
}

/*
 * state_by_name - find a state instance by name
 *
 * @name	The name of the state instance
 */
struct state *state_by_name(const char *name)
{
	struct state *state;

	list_for_each_entry(state, &state_list, list) {
		if (!strcmp(name, state->name))
			return state;
	}

	return NULL;
}

/*
 * state_by_node - find a state instance by of node
 *
 * @node	The of node of the state intance
 */
struct state *state_by_node(const struct device_node *node)
{
	struct state *state;

	list_for_each_entry(state, &state_list, list) {
		if (!strcmp(state->of_path, node->full_name))
			return state;
	}

	return NULL;
}

int state_get_name(const struct state *state, char const **name)
{
	*name = xstrdup(state->name);

	return 0;
}

void state_info(void)
{
	struct state *state;

	printf("registered state instances:\n");

	list_for_each_entry(state, &state_list, list) {
		printf("%-20s ", state->name);
		if (state->backend.format)
			printf("(backend: %s, path: %s)\n",
			       state->backend.format->name,
			       state->backend.of_path);
		else
			printf("(no backend)\n");
	}
}

static void state_shutdown(void)
{
	struct state *state;

	list_for_each_entry(state, &state_list, list) {
		if (state->save_on_shutdown)
			state_save(state);
	}
}
predevshutdown_exitcall(state_shutdown);
