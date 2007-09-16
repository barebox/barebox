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
#endif

int envfs_save(char *filename, char *dirname)
{
	DIR *dir;
	struct dirent *d;
	int malloc_size = 0;
	struct stat s;
	struct envfs_super super;
	void *buf = NULL;
	char tmp[PATH_MAX];
	int isize;
	int envfd;
	struct envfs_inode *inode = NULL;
	int fd;

	envfd = open(filename, O_WRONLY | O_CREAT);
	if (envfd < 0) {
		perror("open");
		return -1;
	}

	super.magic = ENVFS_MAGIC;

	write(envfd, &super, sizeof(struct envfs_super));

	dir = opendir(dirname);
	if (!dir) {
		perror("opendir");
		close(envfd);
		return errno;
	}

	while ((d = readdir(dir))) {
		sprintf(tmp, "%s/%s", dirname, d->d_name);
		if (stat(tmp, &s)) {
			perror("stat");
			goto out;
		}
		if (s.st_mode & S_IFDIR)
			continue;

		isize = ((s.st_size + 3) & ~3) + sizeof(struct envfs_inode);
		if (isize > malloc_size) {
			if (buf)
				free(buf);
			inode = xmalloc(isize);
			buf = inode;
			malloc_size = isize;
		}
		fd = open(tmp, O_RDONLY);
		if (fd < 0) {
			perror("open");
			goto out;
		}
		if (read(fd, inode->data, s.st_size) < s.st_size) {
			perror("read");
			goto out;
		}

		close(fd);
		inode->magic = ENVFS_INODE_MAGIC;
		inode->size  = s.st_size;
		strcpy(inode->name, d->d_name); /* FIXME: strncpy */
		/* FIXME: calc crc */
		if (write(envfd, inode, isize) < isize) {
			perror("write");
			goto out;
		}
	}

	memset(inode, 0, sizeof(struct envfs_inode));
	inode->magic = ENVFS_END_MAGIC;
	if (write(envfd, inode, sizeof(struct envfs_inode)) < sizeof(struct envfs_inode)) {
		perror("write");
		goto out;
	}
out:
	if (buf)
		free(buf);
	close(envfd);
	closedir(dir);
	return 0;
}

#ifdef __U_BOOT__
int do_saveenv(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

	fd = open(filename, O_WRONLY);
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
	if (ret)
		printf("saveenv failed\n");

	ret = protect(fd, ~0, 0, 1);
	/* ENOSYS is no error here, many devices do not need it */
	if (ret && errno != -ENOSYS) {
		printf("could not protect %s: %s\n", filename, errno_str());
		close(fd);
		return 1;
	}

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

int envfs_load(char *filename, char *dirname)
{
	int malloc_size = 0;
	struct envfs_super super;
	void *buf = NULL;
	char tmp[PATH_MAX];
	int envfd;
	struct envfs_inode inode;
	int fd, ret = 0;

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
		sprintf(tmp, "%s/%s", dirname, inode.name);

		fd = open(tmp, O_WRONLY | O_CREAT);
		if (fd < 0) {
			perror("open");
			ret = fd;
			goto out;
		}

		ret = write(fd, buf, inode.size);
		if (ret < inode.size) {
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
int do_loadenv(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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
