/*
 * iomem.c - barebox iomem command
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
#include <command.h>

static void __print_resources(struct resource *res, int indent)
{
	struct resource *r;
	int i;

	for (i = 0; i < indent; i++)
		printf("  ");

	printf(PRINTF_CONVERSION_RESOURCE " - " PRINTF_CONVERSION_RESOURCE
			" (size " PRINTF_CONVERSION_RESOURCE ") %s\n", res->start,
			res->end, resource_size(res), res->name);

	list_for_each_entry(r, &res->children, sibling)
		__print_resources(r, indent + 1);
}

static void print_resources(struct resource *res)
{
	__print_resources(res, 0);
}

static int do_iomem(int argc, char *argv[])
{
	print_resources(&iomem_resource);

	return 0;
}

BAREBOX_CMD_START(iomem)
	.cmd		= do_iomem,
	.usage		= "show iomem usage",
BAREBOX_CMD_END
