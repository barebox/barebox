/*
 * resource.c - barebox resource management
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <init.h>
#include <linux/ioport.h>

static int init_resource(struct resource *res, const char *name)
{
	INIT_LIST_HEAD(&res->children);
	res->parent = NULL;
	res->name = xstrdup(name);

	return 0;
}

/*
 * request a region.
 * This will succedd when the requested region is completely inside
 * the parent resource and does not conflict with any of the child
 * resources.
 */
struct resource *request_region(struct resource *parent,
		const char *name, resource_size_t start,
		resource_size_t end)
{
	struct resource *r, *new;

	if (end < start) {
		debug("%s: request region 0x%08llx:0x%08llx: end < start\n",
				__func__,
				(unsigned long long)start,
				(unsigned long long)end);
		return NULL;
	}

	/* outside parent resource? */
	if (start < parent->start || end > parent->end) {
		debug("%s: 0x%08llx:0x%08llx outside parent resource 0x%08llx:0x%08llx\n",
				__func__,
				(unsigned long long)start,
				(unsigned long long)end,
				(unsigned long long)parent->start,
				(unsigned long long)parent->end);
		return NULL;
	}

	/*
	 * We keep the list of child resources ordered which helps
	 * us searching for conflicts here.
	 */
	list_for_each_entry(r, &parent->children, sibling) {
		if (end < r->start)
			goto ok;
		if (start > r->end)
			continue;
		debug("%s: 0x%08llx:0x%08llx conflicts with 0x%08llx:0x%08llx\n",
				__func__,
				(unsigned long long)start,
				(unsigned long long)end,
				(unsigned long long)r->start,
				(unsigned long long)r->end);
		return NULL;
	}

ok:
	debug("%s ok: 0x%08llx:0x%08llx\n", __func__,
			(unsigned long long)start,
			(unsigned long long)end);

	new = xzalloc(sizeof(*new));
	init_resource(new, name);
	new->start = start;
	new->end = end;
	new->parent = parent;
	list_add_tail(&new->sibling, &r->sibling);

	return new;
}

/*
 * release a region previously requested with request_region
 */
int release_region(struct resource *res)
{
	if (!list_empty(&res->children))
		return -EBUSY;

	list_del(&res->sibling);
	free((char *)res->name);
	free(res);

	return 0;
}

/* The root resource for the whole io space */
struct resource iomem_resource = {
	.start = 0,
	.end = 0xffffffff,
	.name = "iomem",
	.children = LIST_HEAD_INIT(iomem_resource.children),
};

/*
 * request a region inside the io space
 */
struct resource *request_iomem_region(const char *name,
		resource_size_t start, resource_size_t end)
{
	return request_region(&iomem_resource, name, start, end);
}
