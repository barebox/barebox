// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* iomem.c - barebox iomem command */

#include <asm/io.h>
#include <common.h>
#include <command.h>

static void __print_resources(struct resource *res, int indent)
{
	struct resource *r;
	resource_size_t size = resource_size(res);
	int i;

	for (i = 0; i < indent; i++)
		printf("  ");

	printf("%pa - %pa (size %pa) %s%s\n",
			&res->start, &res->end, &size,
			is_reserved_resource(res) ? "[R] " : "",
			res->name);

	list_for_each_entry(r, &res->children, sibling) {
		__print_resources(r, indent + 1);
	}
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
	BAREBOX_CMD_DESC("show IO memory usage")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END

#if IO_SPACE_LIMIT > 0
static int do_ioport(int argc, char *argv[])
{
	print_resources(&ioport_resource);

	return 0;
}

BAREBOX_CMD_START(ioport)
	.cmd		= do_ioport,
	BAREBOX_CMD_DESC("show IO port usage")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
#endif
