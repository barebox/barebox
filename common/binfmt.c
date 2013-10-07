/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#include <common.h>
#include <binfmt.h>
#include <libbb.h>
#include <malloc.h>
#include <command.h>
#include <errno.h>

static LIST_HEAD(binfmt_hooks);

static int binfmt_run(char *file, int argc, char **argv)
{
	struct binfmt_hook *b;
	enum filetype type = file_name_detect_type(file);
	int ret;

	list_for_each_entry(b, &binfmt_hooks, list) {
		if (b->type != type)
			continue;

		ret = b->hook(b, file, argc, argv);
		if (ret != -ERESTARTNOHAND)
			return ret;
	}
	return -ENOENT;
}

/*
 * This converts the original '/executable <args>' into
 * 'barebox_cmd <args> /executable'
 */
static int binfmt_exec_excute(struct binfmt_hook *b, char *file, int argc, char **argv)
{
	char **newargv = xzalloc(sizeof(char*) * (argc + 1));
	int ret, i;

	newargv[0] = b->exec;

	for (i = 1 ; i < argc; i++)
		newargv[i] = argv[i];
	newargv[i] = file;

	ret = execute_binfmt(argc + 1, newargv);

	free(newargv);

	return ret;
}

int execute_binfmt(int argc, char **argv)
{
	int ret;
	char *path;

	if (strchr(argv[0], '/'))
		return binfmt_run(argv[0], argc, argv);

	if (find_cmd(argv[0]))
		return execute_command(argc, &argv[0]);

	path = find_execable(argv[0]);
	if (path) {
		ret = binfmt_run(path, argc, argv);
		free(path);
		return ret;
	}

	return -ENOENT;
}

int binfmt_register(struct binfmt_hook *b)
{
	if (!b || !b->type)
		return -EIO;

	if (!b->hook && !b->exec)
		return -EIO;

	if (b->exec)
		b->hook = binfmt_exec_excute;

	list_add_tail(&b->list, &binfmt_hooks);

	return 0;
}

void binfmt_unregister(struct binfmt_hook *b)
{
	if (!b)
		return;

	list_del(&b->list);
}
