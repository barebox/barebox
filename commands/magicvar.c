#include <common.h>
#include <command.h>
#include <getopt.h>
#include <environment.h>
#include <magicvar.h>
#include <malloc.h>

static LIST_HEAD(magicvars);

struct magicvar_dyn {
	char *name;
	char *description;
	struct list_head list;
};

static void magicvar_print_one(struct magicvar_dyn *md, int verbose)
{
	printf("%-32s %s\n", md->name, md->description);

	if (verbose) {
		const char *val = getenv(md->name);
		if (val && strlen(val))
			printf("  %s\n", val);
	}
}

static struct magicvar_dyn *magicvar_find(const char *name)
{
	struct magicvar_dyn *md;

	list_for_each_entry(md, &magicvars, list)
		if (!strcmp(md->name, name))
			return md;

	return NULL;
}

static void magicvar_remove(struct magicvar_dyn *md)
{
	free(md->name);
	free(md->description);
	list_del(&md->list);
	free(md);
}

static int compare(struct list_head *a, struct list_head *b)
{
	char *na = (char*)list_entry(a, struct magicvar_dyn, list)->name;
	char *nb = (char*)list_entry(b, struct magicvar_dyn, list)->name;

	return strcmp(na, nb);
}

static int magicvar_add(const char *name, const char *description)
{
	struct magicvar_dyn *md;

	md = magicvar_find(name);
	if (md)
		magicvar_remove(md);

	md = xzalloc(sizeof(*md));
	md->name = xstrdup(name);
	md->description = xstrdup(description);

	list_add_sort(&md->list, &magicvars, compare);

	return 0;
}

static void magicvar_build(void)
{
	static int first = 1;
	struct magicvar *m;

	if (!first)
		return;

	for (m = &__barebox_magicvar_start;
			m != &__barebox_magicvar_end;
			m++)
		magicvar_add(m->name, m->description);

	first = 0;
}

static int do_magicvar(int argc, char *argv[])
{
	struct magicvar_dyn *md;
	int opt;
	int verbose = 0, add = 0;

	magicvar_build();

	while ((opt = getopt(argc, argv, "va")) > 0) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		case 'a':
			add = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (add) {
		if (optind + 2 != argc)
			return COMMAND_ERROR_USAGE;

		return magicvar_add(argv[optind], argv[optind + 1]);
	}

	list_for_each_entry(md, &magicvars, list)
		magicvar_print_one(md, verbose);

	return 0;
}


BAREBOX_CMD_HELP_START(magicvar)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-v", "verbose (list all value if there is one)")
BAREBOX_CMD_HELP_OPT ("-a <NAME> <DESC>", "Add a new magicvar")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(magicvar)
	.cmd		= do_magicvar,
	BAREBOX_CMD_DESC("list information about magic variables")
	BAREBOX_CMD_OPTS("[-va]")
	BAREBOX_CMD_HELP(cmd_magicvar_help)
	BAREBOX_CMD_GROUP(CMD_GRP_ENV)
BAREBOX_CMD_END
