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
#else
# define errno_str(x) ("void")
#endif

struct action_data {
	int fd;
	const char *base;
	void *writep;
};

#define PAD4(x) ((x + 3) & ~3) 

static int file_size_action(const char *filename, struct stat *statbuf,
				void *userdata, int depth)
{
	struct action_data *data = userdata;

	data->writep += sizeof(struct envfs_inode);
	data->writep += PAD4(strlen(filename) + 1 - strlen(data->base));
	data->writep += PAD4(statbuf->st_size);
	return 1;
}

static int file_save_action(const char *filename, struct stat *statbuf,
				void *userdata, int depth)
{
	struct action_data *data = userdata;
	struct envfs_inode *inode;
	int fd;
	int namelen = strlen(filename) + 1 - strlen(data->base);

	debug("handling file %s size %ld namelen %d\n", filename + strlen(data->base), statbuf->st_size, namelen);

	inode = (struct envfs_inode*)data->writep;
	inode->magic = ENVFS_INODE_MAGIC;
	inode->namelen = namelen;
	inode->size = statbuf->st_size;
	data->writep += sizeof(struct envfs_inode);

	strcpy(data->writep, filename + strlen(data->base));
	data->writep += PAD4(namelen);

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Open %s %s\n", filename, errno_str());
		goto out;
	}

	if (read(fd, data->writep, statbuf->st_size) < statbuf->st_size) {
		perror("read");
		goto out;
	}
	close(fd);

	data->writep += PAD4(statbuf->st_size);

out:
	return 1;
}

int envfs_save(char *filename, char *dirname)
{
	struct envfs_super *super;
	int envfd, size, ret;
	struct action_data data;
	void *buf = NULL;

	data.writep = 0;
	data.base = dirname;

	/* first pass: calculate size */
	recursive_action(dirname, ACTION_RECURSE, file_size_action,
			NULL, &data, 0);

	size = (unsigned long)data.writep;

	buf = xzalloc(size + sizeof(struct envfs_super));
	data.writep = buf + sizeof(struct envfs_super);

	super = (struct envfs_super *)buf;
	super->magic = ENVFS_MAGIC;
	super->size = size;

	/* second pass: copy files to buffer */
	recursive_action(dirname, ACTION_RECURSE, file_save_action,
			NULL, &data, 0);

	super->crc = crc32(0, buf + sizeof(struct envfs_super), size);
	super->sb_crc = crc32(0, buf, sizeof(struct envfs_super) - 4);

	envfd = open(filename, O_WRONLY | O_CREAT);
	if (envfd < 0) {
		printf("Open %s %s\n", filename, errno_str());
		goto out1;
	}

	if (write(envfd, buf, size  + sizeof(struct envfs_super)) < sizeof(struct envfs_super)) {
		perror("write");
		goto out;
	}

	ret = 0;

out:
	close(envfd);
out1:
	free(buf);
	return ret;
}

#ifdef __U_BOOT__
static int do_saveenv(cmd_tbl_t *cmdtp, int argc, char *argv[])
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

int envfs_load(char *filename, char *dir)
{
	struct envfs_super super;
	void *buf = NULL, *buf_free = NULL;
	int envfd;
	int fd, ret = 0;
	char *str, *tmp;
	int namelen_full;
	unsigned long size;

	envfd = open(filename, O_RDONLY);
	if (envfd < 0) {
		printf("Open %s %s\n", filename, errno_str());
		return -1;
	}

	/* read superblock */
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

	if (crc32(0, (unsigned char *)&super, sizeof(struct envfs_super) - 4)
			!= super.sb_crc) {
		printf("wrong crc on env superblock\n");
		goto out;
	}

	buf = xmalloc(super.size);
	buf_free = buf;
	ret = read(envfd, buf, super.size);
	if (ret < super.size) {
		perror("read");
		goto out;
	}

	if (crc32(0, (unsigned char *)buf, super.size)
			!= super.crc) {
		printf("wrong crc on env\n");
		goto out;
	}

	size = super.size;

	while (size) {
		struct envfs_inode *inode;

		inode = (struct envfs_inode *)buf;
	
		if (inode->magic != ENVFS_INODE_MAGIC) {
			printf("envfs: wrong magic on %s\n", filename);
			ret = -EIO;
			goto out;
		}

		debug("loading %s size %d namelen %d\n", inode->data, inode->size, inode->namelen);

		str = concat_path_file(dir, inode->data);
		tmp = strdup(str);
		make_directory(dirname(tmp));
		free(tmp);

		fd = open(str, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		free(str);
		if (fd < 0) {
			printf("Open %s\n", errno_str());
			ret = fd;
			goto out;
		}

		namelen_full = PAD4(inode->namelen);
		ret = write(fd, buf + namelen_full + sizeof(struct envfs_inode), inode->size);
		if (ret < inode->size) {
			perror("write");
			close(fd);
			goto out;
		}
		close(fd);

		buf += PAD4(inode->namelen) + PAD4(inode->size) + sizeof(struct envfs_inode);
		size -= PAD4(inode->namelen) + PAD4(inode->size) + sizeof(struct envfs_inode);
	}

	ret = 0;
out:
	close(envfd);
	if (buf_free)
		free(buf_free);
	return ret;
}

#ifdef __U_BOOT__
static int do_loadenv(cmd_tbl_t *cmdtp, int argc, char *argv[])
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
