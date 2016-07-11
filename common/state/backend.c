/*
 * Copyright (C) 2016 Pengutronix, Markus Pargmann <mpa@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/string.h>
#include <malloc.h>
#include <printk.h>

#include "state.h"


/**
 * Save the state
 * @param state
 * @return
 */
int state_save(struct state *state)
{
	uint8_t *buf;
	ssize_t len;
	int ret;
	struct state_backend *backend = &state->backend;

	if (!state->dirty)
		return 0;

	ret = backend->format->pack(backend->format, state, &buf, &len);
	if (ret) {
		dev_err(&state->dev, "Failed to pack state with backend format %s, %d\n",
			backend->format->name, ret);
		return ret;
	}

	ret = state_storage_write(&backend->storage, buf, len);
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
 * state_load - Loads a state from the backend
 * @param state The state that should be updated to contain the loaded data
 * @return 0 on success, -errno on failure. If no state is loaded the previous
 * values remain in the state.
 *
 * This function uses the registered storage backend to read data. All data that
 * we read is checked for integrity by the formatter. After that we unpack the
 * data into our state.
 */
int state_load(struct state *state)
{
	uint8_t *buf;
	ssize_t len;
	ssize_t len_hint = 0;
	int ret;
	struct state_backend *backend = &state->backend;

	if (backend->format->get_packed_len)
		len_hint = backend->format->get_packed_len(backend->format,
							   state);
	ret = state_storage_read(&backend->storage, backend->format,
				 state->magic, &buf, &len, len_hint);
	if (ret) {
		dev_err(&state->dev, "Failed to read state with format %s, %d\n",
			backend->format->name, ret);
		return ret;
	}

	ret = backend->format->unpack(backend->format, state, buf, len);
	if (ret) {
		dev_err(&state->dev, "Failed to unpack read data with format %s although verified, %d\n",
			backend->format->name, ret);
		goto out;
	}

	state->dirty = 0;

out:
	free(buf);
	return ret;
}

static int state_format_init(struct state_backend *backend,
			     struct device_d *dev, const char *backend_format,
			     struct device_node *node, const char *state_name)
{
	int ret;

	if (!strcmp(backend_format, "raw")) {
		ret = backend_format_raw_create(&backend->format, node,
						state_name, dev);
	} else if (!strcmp(backend_format, "dtb")) {
		ret = backend_format_dtb_create(&backend->format, dev);
	} else {
		dev_err(dev, "Invalid backend format %s\n",
			backend_format);
		return -EINVAL;
	}

	if (ret && ret != -EPROBE_DEFER)
		dev_err(dev, "Failed to initialize format %s, %d\n",
			backend_format, ret);

	return ret;
}

static void state_format_free(struct state_backend_format *format)
{
	if (format->free)
		format->free(format);
}

/**
 * state_backend_init - Initiates the backend storage and format using the
 * passed arguments
 * @param backend state backend
 * @param dev Device pointer used for prints
 * @param node the DT device node corresponding to the state
 * @param backend_format a string describing the format. Valid values are 'raw'
 * and 'dtb' currently
 * @param storage_path Path to the backend storage file/device/partition/...
 * @param state_name Name of the state
 * @param of_path Path in the devicetree
 * @param stridesize stridesize in case we have a medium without eraseblocks.
 * stridesize describes how far apart copies of the same data should be stored.
 * For blockdevices it makes sense to align them on blocksize.
 * @param storagetype Type of the storage backend. This may be NULL where we
 * autoselect some backwardscompatible backend options
 * @return 0 on success, -errno otherwise
 */
int state_backend_init(struct state_backend *backend, struct device_d *dev,
		       struct device_node *node, const char *backend_format,
		       const char *storage_path, const char *state_name, const
		       char *of_path, off_t offset, size_t max_size,
		       uint32_t stridesize, const char *storagetype)
{
	int ret;

	ret = state_format_init(backend, dev, backend_format, node, state_name);
	if (ret)
		return ret;

	ret = state_storage_init(&backend->storage, dev, storage_path, offset,
				 max_size, stridesize, storagetype);
	if (ret)
		goto out_free_format;

	backend->of_path = of_path;

	return 0;

out_free_format:
	state_format_free(backend->format);
	backend->format = NULL;

	return ret;
}

void state_backend_set_readonly(struct state_backend *backend)
{
	state_storage_set_readonly(&backend->storage);
}

void state_backend_free(struct state_backend *backend)
{
	state_storage_free(&backend->storage);
	if (backend->format)
		state_format_free(backend->format);
}
