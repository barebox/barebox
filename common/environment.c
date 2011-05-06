/*
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

/**
 * @file
 * @brief Environment handling support (host and target)
 *
 * Important: This file will also be used on the host to create
 * the default environment when building the barebox binary. So
 * do not add any new barebox related functions here!
 */

#ifdef __BAREBOX__
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <envfs.h>
#include <xfuncs.h>
#include <libbb.h>
#include <libgen.h>
#include <environment.h>
#else
# define errno_str(x) ("void")
#define EXPORT_SYMBOL(x)
#endif

char *default_environment_path = "/dev/env0";

int file_size_action(const char *filename, struct stat *statbuf,
			    void *userdata, int depth)
{
	struct action_data *data = userdata;

	data->writep += sizeof(struct envfs_inode);
	data->writep += PAD4(strlen(filename) + 1 - strlen(data->base));
	data->writep += PAD4(statbuf->st_size);
	return 1;
}

int file_save_action(const char *filename, struct stat *statbuf,
			    void *userdata, int depth)
{
	struct action_data *data = userdata;
	struct envfs_inode *inode;
	int fd;
	int namelen = strlen(filename) + 1 - strlen(data->base);

	debug("handling file %s size %ld namelen %d\n", filename + strlen(data->base),
		statbuf->st_size, namelen);

	inode = (struct envfs_inode*)data->writep;
	inode->magic = ENVFS_32(ENVFS_INODE_MAGIC);
	inode->namelen = ENVFS_32(namelen);
	inode->size = ENVFS_32(statbuf->st_size);
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

/**
 * Make the current environment persistent
 * @param[in] filename where to store
 * @param[in] dirname what to store (all files in this dir)
 * @return 0 on success, anything else in case of failure
 *
 * Note: This function will also be used on the host! See note in the header
 * of this file.
 */
int envfs_save(char *filename, char *dirname)
{
	struct envfs_super *super;
	int envfd, size, ret;
	struct action_data data;
	void *buf = NULL;

	data.writep = NULL;
	data.base = dirname;

	/* first pass: calculate size */
	recursive_action(dirname, ACTION_RECURSE, file_size_action,
			 NULL, &data, 0);

	size = (unsigned long)data.writep;

	buf = xzalloc(size + sizeof(struct envfs_super));
	data.writep = buf + sizeof(struct envfs_super);

	super = (struct envfs_super *)buf;
	super->magic = ENVFS_32(ENVFS_MAGIC);
	super->size = ENVFS_32(size);

	/* second pass: copy files to buffer */
	recursive_action(dirname, ACTION_RECURSE, file_save_action,
			 NULL, &data, 0);

	super->crc = ENVFS_32(crc32(0, buf + sizeof(struct envfs_super), size));
	super->sb_crc = ENVFS_32(crc32(0, buf, sizeof(struct envfs_super) - 4));

	envfd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (envfd < 0) {
		printf("Open %s %s\n", filename, errno_str());
		ret = envfd;
		goto out1;
	}

	if (write(envfd, buf, size  + sizeof(struct envfs_super)) <
			sizeof(struct envfs_super)) {
		perror("write");
		ret = -1;	/* FIXME */
		goto out;
	}

	ret = 0;

out:
	close(envfd);
out1:
	free(buf);
	return ret;
}
EXPORT_SYMBOL(envfs_save);

/**
 * Restore the last environment into the current one
 * @param[in] filename from where to restore
 * @param[in] dir where to store the last content
 * @return 0 on success, anything else in case of failure
 *
 * Note: This function will also be used on the host! See note in the header
 * of this file.
 */
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

	if ( ENVFS_32(super.magic) != ENVFS_MAGIC) {
		printf("envfs: wrong magic on %s\n", filename);
		ret = -EIO;
		goto out;
	}

	if (crc32(0, (unsigned char *)&super, sizeof(struct envfs_super) - 4)
		   != ENVFS_32(super.sb_crc)) {
		printf("wrong crc on env superblock\n");
		ret = -EIO;
		goto out;
	}

	size = ENVFS_32(super.size);
	buf = xmalloc(size);
	buf_free = buf;
	ret = read(envfd, buf, size);
	if (ret < size) {
		perror("read");
		ret = errno;
		goto out;
	}

	if (crc32(0, (unsigned char *)buf, size)
		     != ENVFS_32(super.crc)) {
		printf("wrong crc on env\n");
		ret = -EIO;
		goto out;
	}

	while (size) {
		struct envfs_inode *inode;
		uint32_t inode_size, inode_namelen;

		inode = (struct envfs_inode *)buf;

		if (ENVFS_32(inode->magic) != ENVFS_INODE_MAGIC) {
			printf("envfs: wrong magic on %s\n", filename);
			ret = -EIO;
			goto out;
		}
		inode_size = ENVFS_32(inode->size);
		inode_namelen = ENVFS_32(inode->namelen);

		debug("loading %s size %d namelen %d\n", inode->data,
			inode_size, inode_namelen);

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

		namelen_full = PAD4(inode_namelen);
		ret = write(fd, buf + namelen_full + sizeof(struct envfs_inode),
				inode_size);
		if (ret < inode_size) {
			perror("write");
			ret = errno;
			close(fd);
			goto out;
		}
		close(fd);

		buf += PAD4(inode_namelen) + PAD4(inode_size) +
				sizeof(struct envfs_inode);
		size -= PAD4(inode_namelen) + PAD4(inode_size) +
				sizeof(struct envfs_inode);
	}

	ret = 0;
out:
	close(envfd);
	if (buf_free)
		free(buf_free);
	return ret;
}
