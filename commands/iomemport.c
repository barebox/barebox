// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* iomem.c - barebox iomem command */

#include <asm/io.h>
#include <common.h>
#include <command.h>
#include <getopt.h>
#include <range.h>

#define	FLAG_VERBOSE		BIT(0)
#define	FLAG_IOPORT		BIT(1)
#define	FLAG_JSON		BIT(2)
#define	FLAG_GAPS		BIT(3)

static inline bool json_puts(const char *str, int flags)
{
	if (flags & FLAG_JSON) {
		puts(str);
		return true;
	}

	return false;
}

static void __print_resources(struct resource *res, int indent,
			      ulong *addr, unsigned flags)
{
	const char *size_str, *name;
	char buf[64];
	struct resource *gap, _gap;
	resource_size_t size = resource_size(res);
	int i;

	gap = flags & FLAG_GAPS ? &_gap : NULL;

	if (addr && !region_overlap_end_inclusive(*addr, *addr, res->start, res->end))
		return;

	if ((flags & FLAG_VERBOSE) && !(flags & (FLAG_IOPORT | FLAG_JSON)))
		printf("%-58s", resource_typeattr_format(buf, sizeof(buf), res) ?: "");

	for (i = 0; i < indent; i++)
		printf("  ");

	name = res->name ?: "";

	if (flags & FLAG_JSON) {
		printf("{ \"name\": \"%s\", \"start\": \"%pa\", "
		       "\"end\": \"%pa\", \"reserved\": %s%s",
		       name, &res->start, &res->end,
		       is_reserved_resource(res) ? "true" : "false",
		       region_is_gap(res) ? ", \"free\": true" : "");
	} else {
		if (flags & (FLAG_VERBOSE | FLAG_IOPORT)) {
			snprintf(buf, sizeof(buf), "%pa", &size);
			size_str = buf;
		} else {
			size_str = size_human_readable(size);
		}

		printf("%pa - %pa (%s %9s) %s%s\n",
		       &res->start, &res->end, region_is_gap(res) ? "free" : "size",
		       size_str, is_reserved_resource(res) ? "[R] " : "", name);
	}

	if (!list_empty(&res->children)) {
		struct resource *r = resource_iter_first(res, gap);

		json_puts(", \"children\": [\n", flags);

		while (r) {
			__print_resources(r, indent + 1, addr, flags);

			r = resource_iter_next(r, gap);
			if (r)
				json_puts(",\n", flags);
		}

		json_puts("]", flags);
	}

	json_puts("}", flags);
}

static void print_resources(struct resource *res, ulong *addr, unsigned flags)
{
	__print_resources(res, 0, addr, flags);
	json_puts("\n", flags);
}

static int do_iomem(int argc, char *argv[])
{
	ulong addr, *arg = NULL;
	unsigned flags = 0;
	int opt, ret;

	while((opt = getopt(argc, argv, "vjg")) > 0) {
		switch(opt) {
		case 'v':
			flags |= FLAG_VERBOSE;
			break;
		case 'j':
			flags |= FLAG_JSON;
			break;
		case 'g':
			flags |= FLAG_GAPS;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc > 1)
		return COMMAND_ERROR_USAGE;

	if (argv[0]) {
		ret = kstrtoul(argv[0], 16, &addr);
		if (ret)
			return ret;
		arg = &addr;
	}

	print_resources(&iomem_resource, arg, flags);

	return 0;
}

BAREBOX_CMD_HELP_START(iomem)
BAREBOX_CMD_HELP_TEXT("Print barebox view of the physical address space.")
BAREBOX_CMD_HELP_TEXT("An optional ADDRESS can be specified to get information")
BAREBOX_CMD_HELP_TEXT("about its region in particular")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-v",  "verbose output")
BAREBOX_CMD_HELP_OPT ("-j",  "JSON output")
BAREBOX_CMD_HELP_OPT ("-g",  "include gaps")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(iomem)
	.cmd		= do_iomem,
	BAREBOX_CMD_DESC("show IO memory usage")
	BAREBOX_CMD_OPTS("[-vjg] [ADDRESS]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
	BAREBOX_CMD_HELP(cmd_iomem_help)
BAREBOX_CMD_END

#if IO_SPACE_LIMIT > 0
static int do_ioport(int argc, char *argv[])
{
	ulong addr, *arg = NULL;
	int ret;

	/* No options supported */
	argv += 1;
	argc -= 1;

	if (argc > 1)
		return COMMAND_ERROR_USAGE;

	if (argv[0]) {
		ret = kstrtoul(argv[0], 16, &addr);
		if (ret)
			return ret;
		arg = &addr;
	}

	print_resources(&ioport_resource, arg, FLAG_IOPORT);

	return 0;
}

BAREBOX_CMD_START(ioport)
	.cmd		= do_ioport,
	BAREBOX_CMD_DESC("show IO port usage")
	BAREBOX_CMD_OPTS("[PORT]")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
#endif
