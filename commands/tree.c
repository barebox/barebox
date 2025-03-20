// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2022 Roger Knecht <rknecht@pm.me>
// SPDX-FileCopyrightText: © 2025 Ahmad Fatoum

#include <stdio.h>
#include <command.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define TREE_GAP	"    "
#define TREE_BAR	"|   "
#define TREE_MID	"|-- "
#define TREE_END	"`-- "

struct tree_buf {
	char buf[80];
};

struct tree_iter {
	struct tree_buf prefix;
	unsigned ndirs;
	unsigned nfiles;
};

static int tree_prefix_append(struct tree_buf *prefix, int pos, const char *suffix)
{
	int strsz = strlen(suffix);

	if (pos + strsz + 1 < sizeof(prefix->buf))
		memcpy(prefix->buf + pos, suffix, strsz + 1);

	return pos + strsz;
}

static int tree(int dirfd, const char *path, int prefix_pos, struct tree_iter *iter)
{
	DIR *dir;
	struct dirent *d;
	int total, ret = 0;

	if (ctrlc())
		return -EINTR;

	dirfd = openat(dirfd, path, O_DIRECTORY);
	if (dirfd < 0)
		return -errno;

	printf("%s\n", path);

	dir = fdopendir(dirfd);
	if (!dir) {
		ret = -errno;
		goto out_close;
	}

	iter->ndirs++;

	total = countdir(dir);
	if (total < 0) {
		ret = -errno;
		goto out_close;
	}

	for (int i = 1; (d = readdir(dir)); i++) {
		struct stat st;
		int pos;

		if (d->d_name[0] == '.')
			continue;

		ret = lstatat(dirfd, d->d_name, &st);
		if (ret)
			st.st_mode = S_IFREG;

		if (i == total)
			pos = tree_prefix_append(&iter->prefix, prefix_pos, TREE_END);
		else
			pos = tree_prefix_append(&iter->prefix, prefix_pos, TREE_MID);

		printf("%*s", pos, iter->prefix.buf);

		switch (st.st_mode & S_IFMT) {
			char realname[PATH_MAX];
		case S_IFLNK:
			realname[PATH_MAX - 1] = '\0';

			printf("%s", d->d_name);
			if (readlinkat(dirfd, d->d_name, realname, PATH_MAX - 1) >= 0)
				printf(" -> %s", realname);
			printf("\n");

			if (statat(dirfd, d->d_name, &st) || !S_ISDIR(st.st_mode))
				iter->nfiles++;
			else
				iter->ndirs++;

			break;
		case S_IFDIR:
			if (i == total)
				pos = tree_prefix_append(&iter->prefix, prefix_pos, TREE_GAP);
			else
				pos = tree_prefix_append(&iter->prefix, prefix_pos, TREE_BAR);

			ret = tree(dirfd, d->d_name, pos, iter);
			if (ret)
				goto out_closedir;
			break;
		default:
			printf("%s\n", d->d_name);
			iter->nfiles++;
		}
	}

out_closedir:
	closedir(dir);
out_close:
	close(dirfd);
	return ret;
}

static int do_tree(int argc, char *argv[])
{
	const char *cwd, **args;
	struct tree_iter iter;
	int ret, exitcode = 0;

	if (argc > 1) {
		args = (const char **)argv + 1;
		argc--;
	} else {
		cwd = getcwd();
		args = &cwd;
	}

	iter.nfiles = iter.ndirs = 0;

	for (int i = 0; i < argc; i++) {
		iter.prefix.buf[0] = 0;
		ret = tree(AT_FDCWD, args[i], 0, &iter);
		if (ret) {
			printf("%s  [error opening dir: %pe]\n", args[i], ERR_PTR(ret));
			exitcode = 1;
		}
	}

	printf("\n%u directories, %u files\n", iter.ndirs, iter.nfiles);

	return exitcode;
}

BAREBOX_CMD_START(tree)
	.cmd		= do_tree,
	BAREBOX_CMD_DESC("list contents of directories in a tree-like format.")
	BAREBOX_CMD_OPTS("[FILEDIR...]")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
BAREBOX_CMD_END
