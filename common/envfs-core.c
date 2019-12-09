// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Albert Schwarzkopf <a.schwarzkopf@phytec.de>, PHYTEC Messtechnik GmbH
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <fs.h>
#include <crc.h>
#include <envfs.h>
#include <libbb.h>
#include <libgen.h>
#include <environment.h>
#include <libfile.h>
#else
# define errno_str(x) ("void")
#endif

static int dir_remove_action(const char *filename, struct stat *statbuf,
		void *userdata, int depth)
{
	if (!depth)
		return 1;

	rmdir(filename);

	return 1;
}

int envfs_check_super(struct envfs_super *super, size_t *size)
{
	if (ENVFS_32(super->magic) != ENVFS_MAGIC) {
		printf("envfs: no envfs (magic mismatch) - envfs never written?\n");
		return -EIO;
	}

	if (crc32(0, super, sizeof(*super) - 4) != ENVFS_32(super->sb_crc)) {
		printf("wrong crc on env superblock\n");
		return -EIO;
	}

	if (super->major < ENVFS_MAJOR)
		printf("envfs version %d.%d loaded into %d.%d\n",
			super->major, super->minor,
			ENVFS_MAJOR, ENVFS_MINOR);

	*size = ENVFS_32(super->size);

	return 0;
}

int envfs_check_data(struct envfs_super *super, const void *buf, size_t size)
{
	uint32_t crc;

	crc = crc32(0, buf, size);
	if (crc != ENVFS_32(super->crc)) {
		printf("wrong crc on env\n");
		return -EIO;
	}

	return 0;
}

int envfs_load_data(struct envfs_super *super, void *buf, size_t size,
		const char *dir, unsigned flags)
{
	int fd, ret = 0;
	char *str, *tmp;
	int headerlen_full;
	/* for envfs < 1.0 */
	struct envfs_inode_end inode_end_dummy;
	struct stat s;

	inode_end_dummy.mode = ENVFS_32(S_IRWXU | S_IRWXG | S_IRWXO);
	inode_end_dummy.magic = ENVFS_32(ENVFS_INODE_END_MAGIC);

	while (size) {
		struct envfs_inode *inode;
		struct envfs_inode_end *inode_end;
		uint32_t inode_size, inode_headerlen, namelen;

		inode = buf;
		buf += sizeof(struct envfs_inode);

		if (ENVFS_32(inode->magic) != ENVFS_INODE_MAGIC) {
			printf("envfs: wrong magic\n");
			ret = -EIO;
			goto out;
		}
		inode_size = ENVFS_32(inode->size);
		inode_headerlen = ENVFS_32(inode->headerlen);
		namelen = strlen(inode->data) + 1;
		if (super->major < 1)
			inode_end = &inode_end_dummy;
		else
			inode_end = buf + PAD4(namelen);

		debug("loading %s size %d namelen %d headerlen %d\n", inode->data,
			inode_size, namelen, inode_headerlen);

		str = concat_path_file(dir, inode->data);

		headerlen_full = PAD4(inode_headerlen);
		buf += headerlen_full;

		if (ENVFS_32(inode_end->magic) != ENVFS_INODE_END_MAGIC) {
			printf("envfs: wrong inode_end_magic\n");
			ret = -EIO;
			goto out;
		}

		tmp = strdup(str);
		make_directory(dirname(tmp));
		free(tmp);

		ret = stat(str, &s);
		if (!ret && (flags & ENV_FLAG_NO_OVERWRITE)) {
			printf("skip %s\n", str);
			goto skip;
		}

		if (S_ISLNK(ENVFS_32(inode_end->mode))) {
			debug("symlink: %s -> %s\n", str, (char*)buf);
			if (!strcmp(buf, basename(str))) {
				unlink(str);
			} else {
				if (!ret)
					unlink(str);

				ret = symlink(buf, str);
				if (ret < 0)
					printf("symlink: %s -> %s : %s\n",
							str, (char*)buf, strerror(-errno));
			}
			free(str);
		} else {
			fd = open(str, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			free(str);
			if (fd < 0) {
				printf("Open %s\n", errno_str());
				ret = fd;
				goto out;
			}

			ret = write(fd, buf, inode_size);
			if (ret < inode_size) {
				perror("write");
				ret = -errno;
				close(fd);
				goto out;
			}
			close(fd);
		}
skip:
		buf += PAD4(inode_size);
		size -= headerlen_full + PAD4(inode_size) +
				sizeof(struct envfs_inode);
	}

	recursive_action(dir, ACTION_RECURSE | ACTION_DEPTHFIRST, NULL,
			dir_remove_action, NULL, 0);

	ret = 0;
out:
	return ret;
}

int envfs_load_from_buf(void *buf, int len, const char *dir, unsigned flags)
{
	int ret;
	size_t size;
	struct envfs_super *super = buf;

	buf = super + 1;

	ret = envfs_check_super(super, &size);
	if (ret)
		return ret;

	ret = envfs_check_data(super, buf, size);
	if (ret)
		return ret;

	ret = envfs_load_data(super, buf, size, dir, flags);

	return ret;
}
