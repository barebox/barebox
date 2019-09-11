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

/**
 * Save the state
 * @param state
 * @return
 */
int state_save(struct state *state)
{
	void *buf;
	ssize_t len;
	int ret;
	struct state_backend_storage_bucket *bucket;
	struct state_backend_storage *storage;

	if (!state->dirty)
		return 0;

	ret = state->format->pack(state->format, state, &buf, &len);
	if (ret) {
		dev_err(&state->dev, "Failed to pack state with backend format %s, %d\n",
			state->format->name, ret);
		return ret;
	}

	storage = &state->storage;
	if (state->keep_prev_content) {
		bool has_content = 0;
		list_for_each_entry(bucket, &storage->buckets, bucket_list)
			has_content |= bucket->wrong_magic;
		if (has_content) {
			dev_err(&state->dev, "Found foreign content on backend, won't overwrite.\n");
			ret = -EPERM;
			goto out;
		}
	}

	ret = state_storage_write(storage, buf, len);
	if (ret) {
		dev_err(&state->dev, "Failed to write packed state, %d\n", ret);
		goto out;
	}

	state->dirty = 0;

out:
	free(buf);
	return ret;
}

/**
 * state_do_load - Loads a state from the backend
 * @param state The state that should be updated to contain the loaded data
 * @return 0 on success, -errno on failure. If no state is loaded the previous
 * values remain in the state.
 *
 * This function uses the registered storage backend to read data. All data that
 * we read is checked for integrity by the formatter. After that we unpack the
 * data into our state.
 */
static int state_do_load(struct state *state, enum state_flags flags)
{
	void *buf;
	ssize_t len;
	int ret;

	ret = state_storage_read(&state->storage, state->format,
				 state->magic, &buf, &len, flags);
	if (ret) {
		dev_err(&state->dev, "Failed to read state with format %s, %d\n",
			state->format->name, ret);
		return ret;
	}

	ret = state->format->unpack(state->format, state, buf, len);
	if (ret) {
		dev_err(&state->dev, "Failed to unpack read data with format %s although verified, %d\n",
			state->format->name, ret);
		goto out;
	}

	state->init_from_defaults = 0;
	state->dirty = 0;

out:
	free(buf);
	return ret;
}

int state_load(struct state *state)
{
	return state_do_load(state, 0);
}

int state_load_no_auth(struct state *state)
{
	return state_do_load(state, STATE_FLAG_NO_AUTHENTICATION);
}

static int state_format_init(struct state *state, const char *backend_format,
			     struct device_node *node, const char *state_name)
{
	int ret;

	if (!backend_format || !strcmp(backend_format, "raw")) {
		ret = backend_format_raw_create(&state->format, node,
						state_name, &state->dev);
	} else if (!strcmp(backend_format, "dtb")) {
		ret = backend_format_dtb_create(&state->format, &state->dev);
	} else {
		dev_err(&state->dev, "Invalid backend format %s\n",
			backend_format);
		return -EINVAL;
	}

	if (ret && ret != -EPROBE_DEFER)
		dev_err(&state->dev, "Failed to initialize format %s, %d\n",
			backend_format, ret);

	return ret;
}

static void state_format_free(struct state_backend_format *format)
{
	if (!format)
		return;

	if (format->free)
		format->free(format);
}

void state_backend_set_readonly(struct state *state)
{
	state_storage_set_readonly(&state->storage);
}

static int state_set_deny(struct param_d *p, void *priv)
{
	return -EROFS;
}

static struct state *state_new(const char *name)
{
	struct state *state;
	int ret;

	state = xzalloc(sizeof(*state));
	dev_set_name(&state->dev, name);
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
	dev_add_param_bool(&state->dev, "dirty", state_set_deny, NULL, &state->dirty,
			   NULL);

	state->save_on_shutdown = 1;
	dev_add_param_bool(&state->dev, "save_on_shutdown", NULL, NULL,
			   &state->save_on_shutdown, NULL);

	dev_add_param_bool(&state->dev, "init_from_defaults", state_set_deny, NULL,
			   &state->init_from_defaults, NULL);

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
		dev_dbg(&state->dev, "Error: invalid variable type '%s'\n", type_name);
		ret = -ENOENT;
		goto out_free;
	}

	if (conv == STATE_CONVERT_FROM_NODE_CREATE) {
		sv = vtype->create(state, name, node, vtype);
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
			ret = of_property_write_string(new_node, "type",
						       vtype->type_name);
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
	struct device_node *new_node, *node, *parent, *backend_node, *aliases;
	struct property *p;
	int ret;
	phandle phandle;

	node = of_find_node_by_path_from(root, state->of_path);
	if (node) {
		/* replace existing node - it will be deleted later */
		parent = node->parent;

		/*
		 * barebox replaces the state node in the device tree it starts the
		 * kernel with, so a state node that already exists in the device tree
		 * will be overwritten. Warn about this so people do not wonder why
		 * changes in the kernels state node do not have any effect.
		 */
		if (root != of_get_root_node())
			dev_warn(&state->dev, "Warning: Kernel devicetree contains state node, replacing it\n");
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
	if (!state->format) {
		ret = -ENODEV;
		goto out;
	}

	p = of_new_property(new_node, "backend-type",
			    state->format->name,
			    strlen(state->format->name) + 1);
	if (!p) {
		ret = -ENOMEM;
		goto out;
	}

	/* backend phandle */
	backend_node = of_find_node_by_reproducible_name(root,
						state->backend_reproducible_name);
	if (!backend_node) {
		ret = -ENODEV;
		goto out;
	}

	phandle = of_node_create_phandle(backend_node);
	ret = of_property_write_u32(new_node, "backend", phandle);
	if (ret)
		goto out;

	if (!strcmp("raw", state->format->name)) {
		struct digest *digest =
		    state_backend_format_raw_get_digest(state->format);
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

	if (state->storage.name) {
		p = of_new_property(new_node, "backend-storage-type",
				    state->storage.name,
				    strlen(state->storage.name) + 1);
		if (!p) {
			ret = -ENOMEM;
			goto out;
		}
	}

	if (state->storage.stridesize) {
		ret = of_property_write_u32(new_node, "backend-stridesize",
				state->storage.stridesize);
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

	aliases = of_create_node(root, "/aliases");
	if (!aliases) {
		ret = -ENOMEM;
		goto out;
	}

	ret = of_set_property(aliases, state->name, new_node->full_name,
			      strlen(new_node->full_name) + 1, 1);
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
	state_storage_free(&state->storage);
	state_format_free(state->format);
	free(state->backend_path);
	free(state->backend_reproducible_name);
	free(state->of_path);
	free(state);
}

/*
 * state_new_from_node - create a new state instance from a device_node
 *
 * @node	The device_node describing the new state instance
 * @readonly	This is a read-only state. Note that with this option set,
 *		there are no repairs done.
 */
struct state *state_new_from_node(struct device_node *node, bool readonly)
{
	struct state *state;
	int ret = 0;
	const char *backend_type;
	const char *storage_type = NULL;
	const char *alias;
	uint32_t stridesize;
	struct device_node *partition_node;
	off_t offset = 0;
	size_t size = 0;

	alias = of_alias_get(node);
	if (!alias) {
		pr_err("State node %s does not have an alias in the /aliases/ node\n", node->full_name);
		return ERR_PTR(-EINVAL);
	}

	state = state_new(alias);
	if (IS_ERR(state))
		return state;

	partition_node = of_parse_phandle(node, "backend", 0);
	if (!partition_node) {
		dev_err(&state->dev, "Cannot resolve \"backend\" phandle\n");
		ret = -EINVAL;
		goto out_release_state;
	}

#ifdef __BAREBOX__
	ret = of_find_path_by_node(partition_node, &state->backend_path, 0);
#else
	ret = of_get_devicepath(partition_node, &state->backend_path, &offset, &size);
#endif
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_err(&state->dev, "state failed to parse path to backend: %s\n",
			       strerror(-ret));
		goto out_release_state;
	}

	state->backend_reproducible_name = of_get_reproducible_name(partition_node);

	ret = of_property_read_string(node, "backend-type", &backend_type);
	if (ret) {
		dev_dbg(&state->dev, "Missing 'backend-type' property\n");
		goto out_release_state;
	}

	ret = of_property_read_u32(node, "backend-stridesize", &stridesize);
	if (ret) {
		dev_dbg(&state->dev, "'backend-stridesize' property undefined, trying to continue without\n");
		stridesize = 0;
	}

	of_property_read_string(node, "backend-storage-type", &storage_type);

	state->keep_prev_content = of_property_read_bool(node,
							"keep-previous-content");

	ret = state_format_init(state, backend_type, node, alias);
	if (ret)
		goto out_release_state;

	ret = state_storage_init(state, state->backend_path, offset,
				 size, stridesize, storage_type);
	if (ret)
		goto out_release_state;

	if (readonly)
		state_backend_set_readonly(state);

	ret = state_from_node(state, node, 1);
	if (ret) {
		goto out_release_state;
	}

	state->init_from_defaults = 1;

	ret = of_register_fixup(of_state_fixup, state);
	if (ret) {
		goto out_release_state;
	}

	dev_info(&state->dev, "New state registered '%s'\n", alias);

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

int state_read_mac(struct state *state, const char *name, u8 *buf)
{
	struct state_variable *svar;
	struct state_mac *mac;

	if (!state || !name || !buf)
		return -EINVAL;

	svar = state_find_var(state, name);
	if (IS_ERR(svar))
		return PTR_ERR(svar);

	if (svar->type->type != STATE_VARIABLE_TYPE_MAC)
		return -EINVAL;

	mac = to_state_mac(svar);
	memcpy(buf, mac->value, 6);

	return 0;
}

void state_info(void)
{
	struct state *state;

	printf("registered state instances:\n");

	list_for_each_entry(state, &state_list, list) {
		printf("%-20s ", state->name);
		if (state->format)
			printf("(backend: %s, path: %s)\n",
			       state->format->name,
			       state->backend_path);
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
