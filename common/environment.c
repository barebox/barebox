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
#include <crc.h>
#include <fcntl.h>
#include <envfs.h>
#include <xfuncs.h>
#include <libbb.h>
#include <libgen.h>
#include <environment.h>
#include <globalvar.h>
#include <libfile.h>
#else
# define errno_str(x) ("void")
#define EXPORT_SYMBOL(x)
#endif

struct envfs_entry {
	char *name;
	void *buf;
	int size;
	struct envfs_entry *next;
	mode_t mode;
};

struct action_data {
	const char *base;
	void *writep;
	struct envfs_entry *env;
};
#define PAD4(x) ((x + 3) & ~3)

#ifdef __BAREBOX__

#define TMPDIR "/.defaultenv"

static char *default_environment_path = "/dev/env0";

void default_environment_path_set(char *path)
{
	default_environment_path = path;
}

char *default_environment_path_get(void)
{
	return default_environment_path;
}

static int do_compare_file(const char *filename, const char *base)
{
	int ret;
	char *cmp;
	const char *relname = filename + strlen(base) + 1;

	cmp = basprintf("%s/%s", TMPDIR, relname);
	ret = compare_file(cmp, filename);

	free(cmp);

	return ret;
}

#else
#define ERASE_SIZE_ALL 0
static inline int protect(int fd, size_t count, unsigned long offset, int prot)
{
	return 0;
}

static inline int erase(int fd, size_t count, unsigned long offset)
{
	return 0;
}

static int do_compare_file(const char *filename, const char *base)
{
	return 1;
}
#endif

static int file_action(const char *filename, struct stat *statbuf,
			    void *userdata, int depth)
{
	struct action_data *data = userdata;
	struct envfs_entry *env;
	int fd, ret;

	if (!do_compare_file(filename, data->base))
		return 1;

	env = xzalloc(sizeof(*env));
	env->name = strdup(filename + strlen(data->base));
	env->size = statbuf->st_size;

	env->buf = calloc(env->size + 1, 1);
	if (!env->buf)
		goto out;

	env->mode = S_IRWXU | S_IRWXG | S_IRWXO;

	if (S_ISLNK(statbuf->st_mode)) {
		env->size++;

		ret = readlink(filename, env->buf, env->size);
		if (ret < 0) {
			perror("read");
			goto out;
		}

		env->mode |= S_IFLNK;
	} else {
		fd = open(filename, O_RDONLY);
		if (fd < 0)
			return fd;

		ret = read(fd, env->buf, env->size);

		close(fd);

		if (ret < env->size) {
			perror("read");
			goto out;
		}
	}

	env->next = data->env;
	data->env = env;
out:
	return 1;
}

static void envfs_save_inode(struct action_data *data, struct envfs_entry *env)
{
	struct envfs_inode *inode;
	struct envfs_inode_end *inode_end;
	int namelen = strlen(env->name) + 1;

	inode = data->writep;
	inode->magic = ENVFS_32(ENVFS_INODE_MAGIC);
	inode->headerlen = ENVFS_32(PAD4(namelen) + sizeof(struct envfs_inode_end));
	inode->size = ENVFS_32(env->size);

	data->writep += sizeof(struct envfs_inode);

	strcpy(data->writep, env->name);
	data->writep += PAD4(namelen);

	inode_end = data->writep;
	inode_end->magic = ENVFS_32(ENVFS_INODE_END_MAGIC);
	inode_end->mode = ENVFS_32(env->mode);
	data->writep += sizeof(struct envfs_inode_end);

	memcpy(data->writep, env->buf, env->size);
	data->writep += PAD4(env->size);
}

#ifdef __BAREBOX__
static int file_remove_action(const char *filename, struct stat *statbuf,
		void *userdata, int depth)
{
	struct action_data *data = userdata;
	char *envname;
	struct stat s;
	int ret;
	struct envfs_entry *env;

	filename += sizeof(TMPDIR) - 1;

	envname = basprintf("%s/%s", data->base, filename);

	ret = stat(envname, &s);
	if (ret) {
		char *base;

		base = basename(envname);

		env = xzalloc(sizeof(*env));
		env->name = strdup(filename);
		env->size = strlen(base) + 1;

		env->buf = strdup(base);
		if (!env->buf)
			goto out;

		env->mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFLNK;

		env->next = data->env;
		data->env = env;
	}
out:
	free(envname);

	return 1;
}
#else
static int file_remove_action(const char *filename, struct stat *statbuf,
		void *userdata, int depth)
{
	return 0;
}
#endif

static int dir_remove_action(const char *filename, struct stat *statbuf,
		void *userdata, int depth)
{
	if (!depth)
		return 1;

	rmdir(filename);

	return 1;
}

/**
 * Make the current environment persistent
 * @param[in] filename where to store
 * @param[in] dirname what to store (all files in this dir)
 * @param[in] flags superblock flags (refer ENVFS_FLAGS_* macros)
 * @return 0 on success, anything else in case of failure
 *
 * Note: This function will also be used on the host! See note in the header
 * of this file.
 */
int envfs_save(const char *filename, const char *dirname, unsigned flags)
{
	struct envfs_super *super;
	int envfd, size, ret;
	struct action_data data = {};
	void *buf = NULL, *wbuf;
	struct envfs_entry *env;
	const char *defenv_path = default_environment_path_get();

	if (!filename)
		filename = defenv_path;
	if (!filename)
		return -ENOENT;

	if (!dirname)
		dirname = "/env";

	data.writep = NULL;
	data.base = dirname;

#ifdef __BAREBOX__
	defaultenv_load(TMPDIR, 0);
#endif

	if (flags & ENVFS_FLAGS_FORCE_BUILT_IN) {
		size = 0; /* force no content */
	} else {
		/* first pass: calculate size */
		recursive_action(dirname, ACTION_RECURSE | ACTION_SORT, file_action,
				NULL, &data, 0);
		recursive_action("/.defaultenv", ACTION_RECURSE | ACTION_SORT,
				file_remove_action, NULL, &data, 0);
		size = 0;

		for (env = data.env; env; env = env->next) {
			size += PAD4(env->size);
			size += sizeof(struct envfs_inode);
			size += PAD4(strlen(env->name) + 1);
			size += sizeof(struct envfs_inode_end);
		}
	}

	buf = xzalloc(size + sizeof(struct envfs_super));
	data.writep = buf + sizeof(struct envfs_super);

	super = buf;
	super->magic = ENVFS_32(ENVFS_MAGIC);
	super->major = ENVFS_MAJOR;
	super->minor = ENVFS_MINOR;
	super->size = ENVFS_32(size);
	super->flags = ENVFS_32(flags);

	if (!(flags & ENVFS_FLAGS_FORCE_BUILT_IN)) {
		/* second pass: copy files to buffer */
		env = data.env;
		while (env) {
			struct envfs_entry *next = env->next;

			envfs_save_inode(&data, env);

			free(env->buf);
			free(env->name);
			free(env);
			env = next;
		}
	}

	super->crc = ENVFS_32(crc32(0, buf + sizeof(struct envfs_super), size));
	super->sb_crc = ENVFS_32(crc32(0, buf, sizeof(struct envfs_super) - 4));

	envfd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (envfd < 0) {
		printf("could not open %s: %s\n", filename, errno_str());
		ret = -errno;
		goto out1;
	}

	ret = protect(envfd, ~0, 0, 0);

	/* ENOSYS and EOPNOTSUPP aren't errors here, many devices don't need it */
	if (ret && errno != ENOSYS && errno != EOPNOTSUPP) {
		printf("could not unprotect %s: %s\n", filename, errno_str());
		goto out;
	}

	ret = erase(envfd, ERASE_SIZE_ALL, 0);

	/* ENOSYS and EOPNOTSUPP aren't errors here, many devices don't need it */
	if (ret && errno != ENOSYS && errno != EOPNOTSUPP) {
		printf("could not erase %s: %s\n", filename, errno_str());
		goto out;
	}

	size += sizeof(struct envfs_super);

	wbuf = buf;

	while (size) {
		ssize_t now = write(envfd, wbuf, size);
		if (now < 0) {
			ret = -errno;
			goto out;
		}

		wbuf += now;
		size -= now;
	}

	ret = protect(envfd, ~0, 0, 1);

	/* ENOSYS and EOPNOTSUPP aren't errors here, many devices don't need it */
	if (ret && errno != ENOSYS && errno != EOPNOTSUPP) {
		printf("could not protect %s: %s\n", filename, errno_str());
		goto out;
	}

	ret = 0;

#ifdef CONFIG_NVVAR
	if (defenv_path && !strcmp(filename, defenv_path))
	    nv_var_set_clean();
#endif
out:
	close(envfd);
out1:
	free(buf);
#ifdef __BAREBOX__
	unlink_recursive(TMPDIR, NULL);
#endif
	return ret;
}
EXPORT_SYMBOL(envfs_save);

static int envfs_check_super(struct envfs_super *super, size_t *size)
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

static int envfs_check_data(struct envfs_super *super, const void *buf, size_t size)
{
	uint32_t crc;

	crc = crc32(0, buf, size);
	if (crc != ENVFS_32(super->crc)) {
		printf("wrong crc on env\n");
		return -EIO;
	}

	return 0;
}

static int envfs_load_data(struct envfs_super *super, void *buf, size_t size,
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

/**
 * Restore the last environment into the current one
 * @param[in] filename from where to restore
 * @param[in] dir where to store the last content
 * @return 0 on success, anything else in case of failure
 *
 * Note: This function will also be used on the host! See note in the header
 * of this file.
 */
int envfs_load(const char *filename, const char *dir, unsigned flags)
{
	struct envfs_super super;
	void *buf = NULL, *rbuf;
	int envfd;
	int ret = 0;
	size_t size, rsize;

	if (!filename)
		filename = default_environment_path_get();
	if (!filename)
		return -ENOENT;

	if (!dir)
		dir = "/env";

	envfd = open(filename, O_RDONLY);
	if (envfd < 0) {
		printf("environment load %s: %s\n", filename, errno_str());
		if (errno == ENOENT)
			printf("Maybe you have to create the partition.\n");
		return -1;
	}

	/* read superblock */
	ret = read(envfd, &super, sizeof(struct envfs_super));
	if ( ret < sizeof(struct envfs_super)) {
		perror("read");
		ret = -errno;
		goto out;
	}

	ret = envfs_check_super(&super, &size);
	if (ret)
		goto out;

	if (super.flags & ENVFS_FLAGS_FORCE_BUILT_IN) {
		printf("found force-builtin environment, using defaultenv\n");
		ret = defaultenv_load(dir, 0);
		if (ret)
			printf("failed to load default environment: %s\n",
					strerror(-ret));
		goto out;
	}

	buf = xmalloc(size);

	rbuf = buf;
	rsize = size;

	while (rsize) {
		ssize_t now;

		now = read(envfd, rbuf, rsize);
		if (now < 0) {
			perror("read");
			ret = -errno;
			goto out;
		}

		if (!now) {
			printf("%s: premature end of file\n", filename);
			ret = -EINVAL;
			goto out;
		}

		rbuf += now;
		rsize -= now;
	}

	ret = envfs_check_data(&super, buf, size);
	if (ret)
		goto out;

	ret = envfs_load_data(&super, buf, size, dir, flags);
	if (ret)
		goto out;

	ret = 0;

out:
	close(envfd);
	free(buf);

	return ret;
}
