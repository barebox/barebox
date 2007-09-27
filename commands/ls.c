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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>

static void ls_one(const char *path, struct stat *s)
{
	char modestr[11];
	unsigned long namelen = strlen(path);

	mkmodestr(s->st_mode, modestr);
	printf("%s %8d %*.*s\n",modestr, s->st_size, namelen, namelen, path);
}

int ls(const char *path, ulong flags)
{
	DIR *dir;
	struct dirent *d;
	char tmp[PATH_MAX];
	struct stat s;

	if (flags & LS_SHOWARG)
		printf("%s:\n", path);

	if (stat(path, &s))
		return errno;

	if (!(s.st_mode & S_IFDIR)) {
		ls_one(path, &s);
		return 0;
	}

	dir = opendir(path);
	if (!dir)
		return errno;

	while ((d = readdir(dir))) {
		sprintf(tmp, "%s/%s", path, d->d_name);
		if (stat(tmp, &s))
			goto out;
		ls_one(d->d_name, &s);
	}

	closedir(dir);

	if (!(flags & LS_RECURSIVE))
		return 0;

	dir = opendir(path);
	if (!dir) {
		errno = -ENOENT;
		return -ENOENT;
	}

	while ((d = readdir(dir))) {

		if (!strcmp(d->d_name, "."))
			continue;
		if (!strcmp(d->d_name, ".."))
			continue;
		sprintf(tmp, "%s/%s", path, d->d_name);

		if (stat(tmp, &s))
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

static int do_ls (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int ret, opt;
	ulong flags = 0;

	getopt_reset();

	while((opt = getopt(argc, argv, "R")) > 0) {
		switch(opt) {
		case 'R':
			flags |= LS_RECURSIVE | LS_SHOWARG;
			break;
		}
	}

	if (argc - optind > 1)
		flags |= LS_SHOWARG;

	if (optind == argc) {
		ret = ls(getcwd(), flags);
		return ret ? 1 : 0;
	}

	while (optind < argc) {
		ret = ls(argv[optind], flags);
		if (ret) {
			perror("ls");
			return 1;
		}
		optind++;
	}

	return 0;
}

static __maybe_unused char cmd_ls_help[] =
"Usage: ls [OPTION]... [FILE]...\n"
"List information about the FILEs (the current directory by default).\n"
"  -R  list subdirectories recursively\n";

U_BOOT_CMD_START(ls)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_ls,
	.usage		= "list a file or directory",
	U_BOOT_CMD_HELP(cmd_ls_help)
U_BOOT_CMD_END
