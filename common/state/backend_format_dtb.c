/*
 * Copyright (C) 2012-2014 Pengutronix, Jan Luebbe <j.luebbe@pengutronix.de>
 * Copyright (C) 2013-2014 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * Copyright (C) 2015 Pengutronix, Marc Kleine-Budde <mkl@pengutronix.de>
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

#include <common.h>
#include <of.h>
#include <linux/kernel.h>
#include <malloc.h>

#include "state.h"

struct state_backend_format_dtb {
	struct state_backend_format format;

	struct device_node *root;

	/* For outputs */
	struct device_d *dev;
};

static inline struct state_backend_format_dtb *get_format_dtb(struct
							      state_backend_format
							      *format)
{
	return container_of(format, struct state_backend_format_dtb, format);
}

static int state_backend_format_dtb_verify(struct state_backend_format *format,
					   uint32_t magic, const void * buf,
					   ssize_t *lenp)
{
	struct state_backend_format_dtb *fdtb = get_format_dtb(format);
	struct device_node *root;
	struct fdt_header *fdt = (struct fdt_header *)buf;
	size_t dtb_len = fdt32_to_cpu(fdt->totalsize);
	size_t len = *lenp;

	if (dtb_len > len) {
		dev_err(fdtb->dev, "Error, stored DTB length (%d) longer than read buffer (%d)\n",
			dtb_len, len);
		return -EINVAL;
	}

	if (fdtb->root) {
		of_delete_node(fdtb->root);
		fdtb->root = NULL;
	}

	root = of_unflatten_dtb(buf);
	if (IS_ERR(root)) {
		dev_err(fdtb->dev, "Failed to unflatten dtb from buffer with length %zd, %ld\n",
			len, PTR_ERR(root));
		return PTR_ERR(root);
	}

	fdtb->root = root;

	*lenp = be32_to_cpu(fdt->totalsize);

	return 0;
}

static int state_backend_format_dtb_unpack(struct state_backend_format *format,
					   struct state *state,
					   const void * buf, ssize_t len)
{
	struct state_backend_format_dtb *fdtb = get_format_dtb(format);
	int ret;

	if (!fdtb->root) {
		state_backend_format_dtb_verify(format, 0, buf, &len);
	}

	ret = state_from_node(state, fdtb->root, 0);
	of_delete_node(fdtb->root);
	fdtb->root = NULL;

	return ret;
}

static int state_backend_format_dtb_pack(struct state_backend_format *format,
					 struct state *state, void ** buf,
					 ssize_t * len)
{
	struct state_backend_format_dtb *fdtb = get_format_dtb(format);
	struct device_node *root;
	struct fdt_header *fdt;

	root = state_to_node(state, NULL, STATE_CONVERT_TO_NODE);
	if (IS_ERR(root)) {
		dev_err(fdtb->dev, "Failed to convert state to device node, %ld\n",
			PTR_ERR(root));
		return PTR_ERR(root);
	}

	fdt = of_flatten_dtb(root);
	if (!fdt) {
		dev_err(fdtb->dev, "Failed to create flattened dtb\n");
		of_delete_node(root);
		return -EINVAL;
	}

	*buf = (uint8_t *) fdt;
	*len = fdt32_to_cpu(fdt->totalsize);

	if (fdtb->root)
		of_delete_node(fdtb->root);
	fdtb->root = root;

	free(fdt);

	return 0;
}

static void state_backend_format_dtb_free(struct state_backend_format *format)
{
	struct state_backend_format_dtb *fdtb = get_format_dtb(format);

	free(fdtb);
}

int backend_format_dtb_create(struct state_backend_format **format,
			      struct device_d *dev)
{
	struct state_backend_format_dtb *dtb;

	dtb = xzalloc(sizeof(*dtb));
	if (!dtb)
		return -ENOMEM;

	dtb->dev = dev;
	dtb->format.pack = state_backend_format_dtb_pack;
	dtb->format.unpack = state_backend_format_dtb_unpack;
	dtb->format.verify = state_backend_format_dtb_verify;
	dtb->format.free = state_backend_format_dtb_free;
	dtb->format.name = "dtb";
	*format = &dtb->format;

	return 0;
}
