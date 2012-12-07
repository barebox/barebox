/*
 * ls.c - list files and directories
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 *
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <stringlist.h>

static void ls_one(const char *path, const char* fullname, struct stat *s)
{
	char modestr[11];
	unsigned int namelen = strlen(path);

	mkmodestr(s->st_mode, modestr);
	printf("%s %10llu %*.*s", modestr, s->st_size, namelen, namelen, path);

	if (S_ISLNK(s->st_mode)) {
		char realname[PATH_MAX];

		memset(realname, 0, PATH_MAX);

		if (readlink(fullname, realname, PATH_MAX - 1) >= 0)
			printf(" -> %s", realname);
	}

	puts("\n");
}

int ls(const char *path, ulong flags)
{
	DIR *dir;
	struct dirent *d;
	char tmp[PATH_MAX];
	struct stat s;
	struct string_list sl;

	string_list_init(&sl);

	if (lstat(path, &s))
		return -errno;

	if (flags & LS_SHOWARG && s.st_mode & S_IFDIR)
		printf("%s:\n", path);

	if (!(s.st_mode & S_IFDIR)) {
		ls_one(path, path, &s);
		return 0;
	}

	dir = opendir(path);
	if (!dir)
		return -errno;

	while ((d = readdir(dir))) {
		sprintf(tmp, "%s/%s", path, d->d_name);
		if (flags & LS_COLUMN) {
			string_list_add_sorted(&sl, d->d_name);
		} else {
			if (lstat(tmp, &s))
				goto out;
			ls_one(d->d_name, tmp, &s);
		}
	}

	closedir(dir);

	if (flags & LS_COLUMN) {
		string_list_print_by_column(&sl);
		string_list_free(&sl);
	}

	if (!(flags & LS_RECURSIVE))
		return 0;

	dir = opendir(path);
	if (!dir) {
		errno = ENOENT;
		return -ENOENT;
	}

	while ((d = readdir(dir))) {

		if (!strcmp(d->d_name, "."))
			continue;
		if (!strcmp(d->d_name, ".."))
			continue;
		sprintf(tmp, "%s/%s", path, d->d_name);

		if (lstat(tmp, &s))
			goto out;
		if (s.st_mode & S_IFDIR) {
			char *norm = normalise_path(tmp);
			ls(norm, flags);
			free(norm);
		}
	}
out:
	closedir(dir);

	return 0;
}

static int do_ls(int argc, char *argv[])
{
	int ret, opt, o;
	struct stat s;
	ulong flags = LS_COLUMN;
	struct string_list sl;

	while((opt = getopt(argc, argv, "RCl")) > 0) {
		switch(opt) {
		case 'R':
			flags |= LS_RECURSIVE | LS_SHOWARG;
			break;
		case 'C':
			flags |= LS_COLUMN;
			break;
		case 'l':
			flags &= ~LS_COLUMN;
			break;
		}
	}

	if (argc - optind > 1)
		flags |= LS_SHOWARG;

	if (optind == argc) {
		ret = ls(getcwd(), flags);
		return ret ? 1 : 0;
	}

	string_list_init(&sl);

	o = optind;

	/* first pass: all files */
	while (o < argc) {
		ret = lstat(argv[o], &s);
		if (ret) {
			printf("%s: %s: %s\n", argv[0],
					argv[o], errno_str());
			o++;
			continue;
		}

		if (!(s.st_mode & S_IFDIR)) {
			if (flags & LS_COLUMN)
				string_list_add_sorted(&sl, argv[o]);
			else
				ls_one(argv[o], argv[o], &s);
		}

		o++;
	}

	if (flags & LS_COLUMN)
		string_list_print_by_column(&sl);

	string_list_free(&sl);

	o = optind;

	/* second pass: directories */
	while (o < argc) {
		ret = lstat(argv[o], &s);
		if (ret) {
			o++;
			continue;
		}

		if (s.st_mode & S_IFDIR) {
			ret = ls(argv[o], flags);
			if (ret) {
				perror("ls");
				o++;
				continue;
			}
		}

		o++;
	}

	return 0;
}

BAREBOX_CMD_HELP_START(ls)
BAREBOX_CMD_HELP_USAGE("ls [OPTIONS] [FILES]\n")
BAREBOX_CMD_HELP_SHORT("List information about the FILEs (the current directory by default).\n")
BAREBOX_CMD_HELP_OPT  ("-R",  "list subdirectories recursively\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ls)
	.cmd		= do_ls,
	.usage		= "list a file or directory",
	BAREBOX_CMD_HELP(cmd_ls_help)
BAREBOX_CMD_END
