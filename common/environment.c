// SPDX-License-Identifier: GPL-2.0-only
/*
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
#include <block.h>
#include <efi/partition.h>
#include <bootsource.h>
#include <magicvar.h>
#else
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

#ifdef __BAREBOX__

#define TMPDIR "/.defaultenv"

static int global_env_autoprobe = IS_ENABLED(CONFIG_INSECURE);
static char *default_environment_path;

void default_environment_path_set(const char *path)
{
	free(default_environment_path);

	default_environment_path = xstrdup(path);
}

static guid_t partition_barebox_env_guid = PARTITION_BAREBOX_ENVIRONMENT_GUID;

/*
 * default_environment_path_search - look for environment partition
 *
 * This searches for a barebox environment partition on block devices. barebox
 * environment partitions are recognized by the guid
 * 6c3737f2-07f8-45d1-ad45-15d260aab24d. The device barebox itself has booted
 * from is preferred over other devices.
 *
 * @return: The cdev providing the environment of found, NULL otherwise
 */
static struct cdev *default_environment_path_search(void)
{
	struct cdev *part;
	struct device_node *boot_node;
	int max_score = 0;
	struct cdev *env_cdev = NULL;
	struct block_device *blk;

	if (!IS_ENABLED(CONFIG_BLOCK) || !global_env_autoprobe)
		return NULL;

	boot_node = bootsource_of_node_get(NULL);

	if (boot_node) {
		struct device *dev;

		dev = of_find_device_by_node(boot_node);
		if (dev)
			device_detect(dev);
	}

	for_each_block_device(blk) {
		int score = 0;

		part = cdev_find_child_by_gpt_typeuuid(&blk->cdev,
						       &partition_barebox_env_guid);
		if (IS_ERR(part))
			continue;

		score++;

		if (boot_node && boot_node == blk->cdev.device_node)
			score++;

		if (score > max_score) {
			max_score = score;
			env_cdev = part;
		}
	}

	return env_cdev;
}

const char *default_environment_path_get(void)
{
	struct cdev *cdev;

	if (default_environment_path)
		return default_environment_path;

	cdev = default_environment_path_search();
	if (cdev)
		default_environment_path = basprintf("/dev/%s", cdev->name);
	else
		default_environment_path = xstrdup("/dev/env0");

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

enum erase_type {
	ERASE_TO_WRITE
};
static inline int erase(int fd, size_t count, unsigned long offset, enum erase_type type)
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
	void *buf = NULL, *wbuf, *bufWithEfi;
	struct envfs_entry *env;
	const char *defenv_path = default_environment_path_get();
	uint32_t magic;

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

	bufWithEfi = xzalloc(size + sizeof(struct envfs_super) +
			     4); // four byte efi attributes
	buf = bufWithEfi + 4;
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

	envfd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (envfd < 0) {
		printf("could not open %s: %m\n", filename);
		ret = -errno;
		goto out1;
	}

	ret = protect(envfd, ~0, 0, 0);

	/* ENOSYS and EOPNOTSUPP aren't errors here, many devices don't need it */
	if (ret && errno != ENOSYS && errno != EOPNOTSUPP) {
		printf("could not unprotect %s: %m\n", filename);
		goto out;
	}

	wbuf = buf;

	/* check if we writing efi vars */
	ret = pread(envfd, &magic, sizeof(uint32_t),
		    4); // four byte efi var attributes
	if (ret == -1 && errno == ENOENT) {
		// skip as file don't exist
		goto skip_efi_read;
	}
	if (ret < sizeof(u_int32_t)) {
		perror("read of destination file failed");
		ret = -errno;
		goto skip_efi_read;
	}

	if (ENVFS_32(magic) == ENVFS_MAGIC) {
		pr_info("looks like we writing efi vars, keep attributes\n");
		ret = pread(envfd, bufWithEfi, sizeof(uint32_t), 0);
		if (ret < sizeof(uint32_t)) {
			perror("read of efi attributes failed");
			ret = -errno;
			goto out;
		}
		size += sizeof(uint32_t);
		wbuf = bufWithEfi;
	}

skip_efi_read:
	ret = erase(envfd, ERASE_SIZE_ALL, 0, ERASE_TO_WRITE);

	/* ENOSYS and EOPNOTSUPP aren't errors here, many devices don't need it */
	if (ret && errno != ENOSYS && errno != EOPNOTSUPP) {
		printf("could not erase %s: %m\n", filename);
		goto out;
	}

	size += sizeof(struct envfs_super);

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
		printf("could not protect %s: %m\n", filename);
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
	free(bufWithEfi);
#ifdef __BAREBOX__
	unlink_recursive(TMPDIR, NULL);
#endif
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
int envfs_load(const char *filename, const char *dir, unsigned flags)
{
	struct envfs_super super;
	void *buf = NULL, *rbuf;
	int envfd;
	int ret = 0;
	size_t size, rsize;
	uint32_t magic;

	if (!filename)
		filename = default_environment_path_get();
	if (!filename)
		return -ENOENT;

	if (!dir)
		dir = "/env";

	envfd = open(filename, O_RDONLY);
	if (envfd < 0) {
		printf("environment load %s: %m\n", filename);
		if (errno == ENOENT)
			printf("Maybe you have to create the partition.\n");
		return -1;
	}

	/* check if we reading efi vars */
	ret = pread(envfd, &magic, sizeof(uint32_t),
		    4); // four byte efi var attributes
	if (ret < sizeof(u_int32_t)) {
		perror("read");
		ret = -errno;
		goto out;
	}

	if (ENVFS_32(magic) == ENVFS_MAGIC) {
		pr_info("looks like we reading efi vars, skip attributes\n");
		ret = read(envfd, &magic,
			   sizeof(uint32_t)); // simply reuse the memory
		if (ret < sizeof(uint32_t)) {
			perror("read");
			ret = -errno;
			goto out;
		}
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

#ifdef __BAREBOX__
static int register_env_vars(void)
{
	globalvar_add_simple_bool("env.autoprobe", &global_env_autoprobe);
	return 0;
}
postcore_initcall(register_env_vars);
BAREBOX_MAGICVAR(global.env.autoprobe,
                 "Automatically probe known block devices for environment");
#endif
