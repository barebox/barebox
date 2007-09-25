/*
 * environment.c - simple archive implementation
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

#ifdef __U_BOOT__
#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <envfs.h>
#include <xfuncs.h>
#include <libbb.h>
#include <libgen.h>
#endif

struct action_data {
	int fd;
	const char *base;
};

static int file_save_action(const char *filename, struct stat *statbuf,
				void *userdata, int depth)
{
	int isize;
	struct action_data *data = userdata;
	struct envfs_inode *inode = NULL;
	int fd;
	int namelen = strlen(filename) + 1 - strlen(data->base);
	int namelen_full = (namelen + 3) & ~3;

	isize = namelen_full + ((statbuf->st_size + 3) & ~3) + sizeof(struct envfs_inode);
	inode = xzalloc(isize);

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("open");
		goto out;
	}

	if (read(fd, inode->data + namelen_full, statbuf->st_size) < statbuf->st_size) {
		perror("read");
		goto out;
	}

	close(fd);
	inode->magic = ENVFS_INODE_MAGIC;
	inode->namelen = namelen;
	inode->size  = namelen_full + statbuf->st_size;

	strcpy(inode->data, filename + strlen(data->base));

	/* FIXME: calc crc */

	if (write(data->fd, inode, isize) < isize) {
		perror("write");
		goto out;
	}
out:
	free(inode);
	return 1;
}

int envfs_save(char *filename, char *dirname)
{
	struct envfs_super super;
	int envfd;
	struct envfs_inode inode;
	struct action_data data;

	envfd = open(filename, O_WRONLY | O_CREAT);
	if (envfd < 0) {
		perror("open");
		return -1;
	}

	super.magic = ENVFS_MAGIC;

	write(envfd, &super, sizeof(struct envfs_super));

	data.fd = envfd;
	data.base = dirname;

	recursive_action(dirname, ACTION_RECURSE, file_save_action,
			NULL, &data, 0);

	memset(&inode, 0, sizeof(struct envfs_inode));
	inode.magic = ENVFS_END_MAGIC;
	if (write(envfd, &inode, sizeof(struct envfs_inode)) < sizeof(struct envfs_inode)) {
		perror("write");
		goto out;
	}
out:
	close(envfd);
	return 0;
}

#ifdef __U_BOOT__
int do_saveenv(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int ret, fd;
	char *filename, *dirname;

	printf("saving environment\n");
	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = "/dev/env0";
	else
		filename = argv[1];

	fd = open(filename, O_WRONLY | O_CREAT);
	if (fd < 0) {
		printf("could not open %s: %s", filename, errno_str());
		return 1;
	}

	ret = protect(fd, ~0, 0, 0);

	/* ENOSYS is no error here, many devices do not need it */
	if (ret && errno != -ENOSYS) {
		printf("could not unprotect %s: %s\n", filename, errno_str());
		close(fd);
		return 1;
	}

	ret = erase(fd, ~0, 0);

	/* ENOSYS is no error here, many devices do not need it */
	if (ret && errno != -ENOSYS) {
		printf("could not erase %s: %s\n", filename, errno_str());
		close(fd);
		return 1;
	}

	ret = envfs_save(filename, dirname);
	if (ret) {
		printf("saveenv failed\n");
		goto out;
	}

	ret = protect(fd, ~0, 0, 1);

	/* ENOSYS is no error here, many devices do not need it */
	if (ret && errno != -ENOSYS) {
		printf("could not protect %s: %s\n", filename, errno_str());
		close(fd);
		return 1;
	}

	ret = 0;
out:
	close(fd);
	return ret;
}

static __maybe_unused char cmd_saveenv_help[] =
"Usage: saveenv [DIRECTORY] [ENVFS]\n"
"Save the files in <directory> to the persistent storage device <envfs>.\n"
"<envfs> is normally a block in flash, but could be any other file.\n"
"If ommitted <directory> defaults to /env and <envfs> defaults to /dev/env0.\n"
"Note that envfs can only handle files. Directories are skipped silently.\n";

U_BOOT_CMD_START(saveenv)
	.maxargs	= 3,
	.cmd		= do_saveenv,
	.usage		= "save environment to persistent storage",
	U_BOOT_CMD_HELP(cmd_saveenv_help)
U_BOOT_CMD_END
#endif /* __U_BOOT__ */

int mkdirp(const char *dir)
{
	char *s = strdup(dir);
	char *path = s;
	char c;

	do {
		c = 0;

		/* Bypass leading non-'/'s and then subsequent '/'s. */
		while (*s) {
			if (*s == '/') {
				do {
					++s;
				} while (*s == '/');
				c = *s;		/* Save the current char */
				*s = 0;		/* and replace it with nul. */
				break;
			}
			++s;
		}

		if (mkdir(path, 0777) < 0) {

			/* If we failed for any other reason than the directory
			 * already exists, output a diagnostic and return -1.*/
#ifdef __U_BOOT__
			if (errno != -EEXIST)
#else
			if (errno != EEXIST)
#endif
				break;
		}
		if (!c)
			goto out;

		/* Remove any inserted nul from the path (recursive mode). */
		*s = c;

	} while (1);

out:
	free(path);
	return errno;
}

int envfs_load(char *filename, char *dir)
{
	int malloc_size = 0;
	struct envfs_super super;
	void *buf = NULL;
	int envfd;
	struct envfs_inode inode;
	int fd, ret = 0;
	char *str, *tmp;
	int namelen_full;

	envfd = open(filename, O_RDONLY);
	if (envfd < 0) {
		perror("open");
		return -1;
	}

	ret = read(envfd, &super, sizeof(struct envfs_super));
	if ( ret < sizeof(struct envfs_super)) {
		perror("read");
		ret = errno;
		goto out;
	}

	if (super.magic != ENVFS_MAGIC) {
		printf("envfs: wrong magic on %s\n", filename);
		ret = -EIO;
		goto out;
	}

	while (1) {
		ret = read(envfd, &inode, sizeof(struct envfs_inode));
		if (ret < sizeof(struct envfs_inode)) {
			perror("read");
			goto out;
		}
		if (inode.magic == ENVFS_END_MAGIC) {
			ret = 0;
			break;
		}
		if (inode.magic != ENVFS_INODE_MAGIC) {
			printf("envfs: wrong magic on %s\n", filename);
			ret = -EIO;
			goto out;
		}
		if (inode.size > malloc_size) {
			if (buf)
				free(buf);
			buf = xmalloc(inode.size);
			malloc_size = inode.size;
		}

		ret = read(envfd, buf, inode.size);
		if (ret < inode.size) {
			perror("read");
			goto out;
		}

		str = concat_path_file(dir, buf);
		tmp = strdup(str);
		mkdirp(dirname(tmp));
		free(tmp);

		fd = open(str, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		free(str);
		if (fd < 0) {
			perror("open");
			ret = fd;
			goto out;
		}

		namelen_full = ((inode.namelen + 3) & ~3);
		ret = write(fd, buf + namelen_full, inode.size - namelen_full);
		if (ret < inode.size - namelen_full) {
			perror("write");
			close(fd);
			goto out;
		}
		close(fd);

		if (inode.size & 0x3) {
			ret = read(envfd, buf, 4 - (inode.size & 0x3));
			if (ret < 4 - (inode.size & 0x3)) {
				perror("read");
				ret = errno;
				goto out;
			}
		}
	}

out:
	close(envfd);
	if (buf)
		free(buf);
	return ret;
}

#ifdef __U_BOOT__
int do_loadenv(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	char *filename, *dirname;

	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = "/dev/env0";
	else
		filename = argv[1];
	printf("loading environment from %s\n", filename);
	return envfs_load(filename, dirname);
}

static __maybe_unused char cmd_loadenv_help[] =
"Usage: loadenv [ENVFS] [DIRECTORY]\n"
"Load the persistent storage contained in <envfs> to the directory\n"
"<directory>.\n"
"If ommitted <directory> defaults to /env and <envfs> defaults to /dev/env0.\n"
"Note that envfs can only handle files. Directories are skipped silently.\n";

U_BOOT_CMD_START(loadenv)
	.maxargs	= 3,
	.cmd		= do_loadenv,
	.usage		= "load environment from persistent storage",
	U_BOOT_CMD_HELP(cmd_loadenv_help)
U_BOOT_CMD_END
#endif /* __U_BOOT__ */
