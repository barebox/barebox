/*
 * fs.c - posix like file functions
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <driver.h>
#include <errno.h>
#include <malloc.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <fcntl.h>
#include <xfuncs.h>
#include <init.h>
#include <module.h>
#include <libbb.h>
#include <magicvar.h>
#include <environment.h>
#include <libgen.h>
#include <block.h>
#include <slice.h>
#include <libfile.h>
#include <parseopt.h>
#include <linux/namei.h>

char *mkmodestr(unsigned long mode, char *str)
{
	static const char *l = "xwr";
	int mask = 1, i;
	char c;

	switch (mode & S_IFMT) {
		case S_IFDIR:    str[0] = 'd'; break;
		case S_IFBLK:    str[0] = 'b'; break;
		case S_IFCHR:    str[0] = 'c'; break;
		case S_IFIFO:    str[0] = 'f'; break;
		case S_IFLNK:    str[0] = 'l'; break;
		case S_IFSOCK:   str[0] = 's'; break;
		case S_IFREG:    str[0] = '-'; break;
		default:         str[0] = '?';
	}

	for(i = 0; i < 9; i++) {
		c = l[i%3];
		str[9-i] = (mode & mask)?c:'-';
		mask = mask<<1;
	}

	if(mode & S_ISUID) str[3] = (mode & S_IXUSR)?'s':'S';
	if(mode & S_ISGID) str[6] = (mode & S_IXGRP)?'s':'S';
	if(mode & S_ISVTX) str[9] = (mode & S_IXOTH)?'t':'T';
	str[10] = '\0';
	return str;
}
EXPORT_SYMBOL(mkmodestr);

void cdev_print(const struct cdev *cdev)
{
	struct device_node *np;
	int nbytes;

	if (cdev->dev || cdev->master || cdev->partname) {
		printf("Origin: %s", dev_name(cdev->dev) ?: "None");
		if (cdev->master)
			printf("\tMaster: %s", cdev->master->name);
		if (cdev->partname)
			printf("\tPartition: %s", cdev->partname);
		printf("\n");
	}
	printf("Ocount: %d\tFlags: 0x%02x", cdev->open, cdev->flags);
	if (cdev->flags) {
		printf(" (");
		if (cdev->flags & DEVFS_IS_CHARACTER_DEV)
			printf(" cdev");
		if (cdev->flags & DEVFS_PARTITION_FIXED)
			printf(" fixed-partition");
		if (cdev->flags & DEVFS_PARTITION_READONLY)
			printf(" readonly-partition");
		if (cdev->flags & DEVFS_PARTITION_FROM_OF)
			printf(" of-partition");
		if (cdev->flags & DEVFS_PARTITION_FROM_TABLE)
			printf(" table-partition");
		if (cdev->flags & DEVFS_IS_MCI_MAIN_PART_DEV)
			printf(" mci-main-partition");
		if (cdev->flags & DEVFS_IS_MBR_PARTITIONED)
			printf(" mbr-partitioned");
		if (cdev->flags & DEVFS_IS_GPT_PARTITIONED)
			printf(" gpt-partitioned");
		if (cdev->mtd)
			printf(" mtd");
		if (cdev->flags & DEVFS_PARTITION_BOOTABLE_ESP)
			printf(" boot-esp");
		if (cdev->flags & DEVFS_PARTITION_BOOTABLE_LEGACY)
			printf(" boot-legacy");
		printf(" )");
	}
	printf("\n");

	nbytes = 0;

	if (cdev->filetype)
		nbytes += printf("Filetype: %s\t", file_type_to_string(cdev->filetype));
	if (cdev_is_mbr_partitioned(cdev->master))
		nbytes += printf("DOS parttype: 0x%02x\t", cdev->dos_partition_type);
	else if (cdev_is_gpt_partitioned(cdev->master))
		nbytes += printf("GPT typeuuid: %pUl\t", &cdev->typeuuid);
	if (*cdev->partuuid || *cdev->diskuuid)
		nbytes += printf("%sUUID: %s", cdev_is_partition(cdev) ? "PART" : "DISK",
				 cdev_is_partition(cdev) ? cdev->partuuid : cdev->diskuuid);

	if (nbytes)
		printf("\n");

	np = cdev_of_node(cdev);
	if (np)
		printf("DT node: %pOF\n", np);
}
EXPORT_SYMBOL(cdev_print);

static struct fs_device *get_fsdevice_by_path(int dirfd, const char *path);

void stat_print(const char *filename, const struct stat *st)
{
	int dirfd = AT_FDCWD;
	struct block_device *bdev = NULL;
	struct fs_device *fdev;
	struct cdev *cdev = NULL;
	const char *type = NULL, *typeprefix = "";
	bool is_cdev_link = false;
	char modestr[11];

	mkmodestr(st->st_mode, modestr);

	switch (st->st_mode & S_IFMT) {
		case S_IFDIR:    type = "directory"; break;
		case S_IFBLK:    type = "block special file"; break;
		case S_IFCHR:    type = "character special file"; break;
		case S_IFIFO:    type = "fifo"; break;
		case S_IFLNK:    type = "symbolic link"; break;
		case S_IFSOCK:   type = "socket"; break;
		case S_IFREG:    type = "regular file"; break;
	}

	if (st->st_mode & S_IFCHR) {
		char *path;

		path = canonicalize_path(dirfd, filename);
		if (path) {
			const char *devicefile = devpath_to_name(path);
			struct cdev *lcdev;

			lcdev = lcdev_by_name(devicefile);
			cdev = cdev_readlink(lcdev);
			if (cdev != lcdev)
				is_cdev_link = true;
			if (cdev)
				bdev = cdev_get_block_device(cdev);

			free(path);
		}
	}

	printf("  File: %s", filename);

	if (S_ISLNK(st->st_mode)) {
		char realname[PATH_MAX] = {};
		int ret;

		ret = readlink(filename, realname, PATH_MAX - 1);
		if (ret)
			printf(" -> <readlink error %pe>", ERR_PTR(ret));
		else
			printf(" -> %s", realname);
	} else if (is_cdev_link) {
		printf(" ~> %s", cdev->name);
		typeprefix = "cdev link to ";
	}

	printf("\n");

	printf("  Size: %-20llu", st->st_size);
	if (bdev)
		printf("Blocks: %llu\tIO Block: %u\t",
		       (u64)bdev->num_blocks, 1 << bdev->blockbits);

	if (type)
		printf("  %s%s", typeprefix, type);

	fdev = get_fsdevice_by_path(dirfd, filename);

	printf("\nDevice: %s\tInode: %lu\n",
	       fdev ? dev_name(&fdev->dev) : "<unknown>",
	       st->st_ino);
	printf("Access: (%04o/%s)\tUid: (%u)\tGid: (%u)\n",
	       st->st_mode & 07777, modestr, st->st_uid, st->st_gid);

	if (cdev)
		cdev_print(cdev);
}
EXPORT_SYMBOL(stat_print);

static char *cwd;
static struct dentry *cwd_dentry;
static struct vfsmount *cwd_mnt;

static FILE *files;
static struct dentry *d_root;
static struct vfsmount *mnt_root;

static struct fs_driver *ramfs_driver;

static int init_fs(void)
{
	cwd = xzalloc(PATH_MAX);
	*cwd = '/';

	files = xzalloc(sizeof(FILE) * MAX_FILES);

	return 0;
}

postcore_initcall(init_fs);

struct filename;

static int filename_lookup(int dirfd, struct filename *name, unsigned flags,
			   struct path *path);;
static struct filename *getname(const char *filename);
static void path_put(const struct path *path);

LIST_HEAD(fs_device_list);

static struct vfsmount *mntget(struct vfsmount *mnt)
{
	if (!mnt)
		return NULL;

	mnt->ref++;

	return mnt;
}

static void mntput(struct vfsmount *mnt)
{
	if (!mnt)
		return;

	mnt->ref--;
}

static struct vfsmount *lookup_mnt(struct path *path)
{
	struct fs_device *fsdev;

	for_each_fs_device(fsdev) {
		if (path->dentry == fsdev->vfsmount.mountpoint) {
			mntget(&fsdev->vfsmount);
			return &fsdev->vfsmount;
		}
	}

	return NULL;
}

/*
 * get_cdev_by_mountpath - return the cdev the given path
 *                         is mounted on
 */
struct cdev *get_cdev_by_mountpath(const char *path)
{
	struct fs_device *fsdev;

	fsdev = get_fsdevice_by_path(AT_FDCWD, path);
	if (!fsdev)
		return NULL;

	return fsdev->cdev;
}

char *get_mounted_path(const char *path)
{
	struct fs_device *fdev;

	fdev = get_fsdevice_by_path(AT_FDCWD, path);
	if (!fdev)
		return NULL;

	return fdev->path;
}

static FILE *get_file(void)
{
	int i;

	for (i = 3; i < MAX_FILES; i++) {
		if (!files[i].in_use) {
			memset(&files[i], 0, sizeof(FILE));
			files[i].in_use = 1;
			files[i].no = i;
			return &files[i];
		}
	}
	return NULL;
}

static void put_file(FILE *f)
{
	free(f->path);
	f->path = NULL;
	f->in_use = 0;
	iput(f->f_inode);
	dput(f->dentry);
}

static FILE *fd_to_file(int fd, bool o_path_ok)
{
	if (fd < 0 || fd >= MAX_FILES || !files[fd].in_use) {
		errno = EBADF;
		return ERR_PTR(-errno);
	}
	if (!o_path_ok && (files[fd].flags & O_PATH)) {
		errno = EINVAL;
		return ERR_PTR(-errno);
	}

	return &files[fd];
}

static int create(struct dentry *dir, struct dentry *dentry)
{
	struct inode *inode;

	if (d_is_negative(dir))
		return -ENOENT;

	inode = d_inode(dir);

	if (!inode->i_op->create)
		return -EROFS;

	return inode->i_op->create(inode, dentry, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
}

static int fsdev_truncate(struct device *dev, FILE *f, loff_t length)
{
	struct fs_driver *fsdrv = f->fsdev->driver;

	return fsdrv->truncate ? fsdrv->truncate(dev, f, length) : -EROFS;
}

int ftruncate(int fd, loff_t length)
{
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;

	if (f->size == FILE_SIZE_STREAM)
		return 0;

	ret = fsdev_truncate(&f->fsdev->dev, f, length);
	if (ret)
		return errno_set(ret);

	f->size = length;
	f->f_inode->i_size = f->size;

	return 0;
}

int ioctl(int fd, int request, void *buf)
{
	struct fs_driver *fsdrv;
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;

	fsdrv = f->fsdev->driver;

	if (fsdrv->ioctl)
		ret = fsdrv->ioctl(&f->fsdev->dev, f, request, buf);
	else
		ret = -ENOSYS;

	return errno_set(ret);
}

static ssize_t __read(FILE *f, void *buf, size_t count)
{
	struct fs_driver *fsdrv;
	int ret;

	if ((f->flags & O_ACCMODE) == O_WRONLY) {
		ret = -EBADF;
		goto out;
	}

	fsdrv = f->fsdev->driver;

	if (fsdrv != ramfs_driver)
		assert_command_context();

	if (f->size != FILE_SIZE_STREAM && f->pos + count > f->size)
		count = f->size - f->pos;

	if (!count)
		return 0;

	ret = fsdrv->read(&f->fsdev->dev, f, buf, count);
out:
	return errno_set(ret);
}

ssize_t pread(int fd, void *buf, size_t count, loff_t offset)
{
	loff_t pos;
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;

	pos = f->pos;
	f->pos = offset;
	ret = __read(f, buf, count);
	f->pos = pos;

	return ret;
}
EXPORT_SYMBOL(pread);

ssize_t read(int fd, void *buf, size_t count)
{
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;

	ret = __read(f, buf, count);

	if (ret > 0)
		f->pos += ret;
	return ret;
}
EXPORT_SYMBOL(read);

static ssize_t __write(FILE *f, const void *buf, size_t count)
{
	struct fs_driver *fsdrv;
	int ret;

	fsdrv = f->fsdev->driver;

	if ((f->flags & O_ACCMODE) == O_RDONLY || !fsdrv->write) {
		ret = -EBADF;
		goto out;
	}

	if (fsdrv != ramfs_driver)
		assert_command_context();

	if (f->size != FILE_SIZE_STREAM && f->pos + count > f->size) {
		ret = fsdev_truncate(&f->fsdev->dev, f, f->pos + count);
		if (ret) {
			if (ret == -EPERM)
				ret = -ENOSPC;
			if (ret != -ENOSPC)
				goto out;
			count = f->size - f->pos;
			if (!count)
				goto out;
		} else {
			f->size = f->pos + count;
			f->f_inode->i_size = f->size;
		}
	}
	ret = fsdrv->write(&f->fsdev->dev, f, buf, count);
out:
	return errno_set(ret);
}

ssize_t pwrite(int fd, const void *buf, size_t count, loff_t offset)
{
	loff_t pos;
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;

	pos = f->pos;
	f->pos = offset;
	ret = __write(f, buf, count);
	f->pos = pos;

	return ret;
}
EXPORT_SYMBOL(pwrite);

ssize_t write(int fd, const void *buf, size_t count)
{
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;

	ret = __write(f, buf, count);

	if (ret > 0)
		f->pos += ret;
	return ret;
}
EXPORT_SYMBOL(write);

int flush(int fd)
{
	struct fs_driver *fsdrv;
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;

	fsdrv = f->fsdev->driver;
	if (fsdrv->flush)
		ret = fsdrv->flush(&f->fsdev->dev, f);
	else
		ret = 0;

	return errno_set(ret);
}

loff_t lseek(int fd, loff_t offset, int whence)
{
	struct fs_driver *fsdrv;
	FILE *f = fd_to_file(fd, false);
	loff_t pos;
	int ret;

	if (IS_ERR(f))
		return -1;

	fsdrv = f->fsdev->driver;

	if (fsdrv != ramfs_driver)
		assert_command_context();

	ret = -EINVAL;

	switch (whence) {
	case SEEK_SET:
		pos = 0;
		break;
	case SEEK_CUR:
		pos = f->pos;
		break;
	case SEEK_END:
		pos = f->size;
		break;
	default:
		goto out;
	}

	pos += offset;

	if (f->size != FILE_SIZE_STREAM && (pos < 0 || pos > f->size))
		goto out;

	if (fsdrv->lseek) {
		ret = fsdrv->lseek(&f->fsdev->dev, f, pos);
		if (ret < 0)
			goto out;
	}

	f->pos = pos;

	return pos;

out:
	errno_set(ret);

	return -1;
}
EXPORT_SYMBOL(lseek);

int erase(int fd, loff_t count, loff_t offset)
{
	struct fs_driver *fsdrv;
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;
	if (offset >= f->size)
		return 0;
	if (count == ERASE_SIZE_ALL || count > f->size - offset)
		count = f->size - offset;
	if (count < 0)
		return -EINVAL;

	fsdrv = f->fsdev->driver;

	if (fsdrv != ramfs_driver)
		assert_command_context();

	if (fsdrv->erase)
		ret = fsdrv->erase(&f->fsdev->dev, f, count, offset);
	else
		ret = -ENOSYS;

	return errno_set(ret);
}
EXPORT_SYMBOL(erase);

int protect(int fd, size_t count, loff_t offset, int prot)
{
	struct fs_driver *fsdrv;
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;
	if (offset >= f->size)
		return 0;
	if (count > f->size - offset)
		count = f->size - offset;

	fsdrv = f->fsdev->driver;

	if (fsdrv != ramfs_driver)
		assert_command_context();

	if (fsdrv->protect)
		ret = fsdrv->protect(&f->fsdev->dev, f, count, offset, prot);
	else
		ret = -ENOSYS;

	return errno_set(ret);
}
EXPORT_SYMBOL(protect);

int discard_range(int fd, loff_t count, loff_t offset)
{
	struct fs_driver *fsdrv;
	FILE *f = fd_to_file(fd, false);
	int ret;

	if (IS_ERR(f))
		return -errno;
	if (offset >= f->size)
		return 0;
	if (count > f->size - offset)
		count = f->size - offset;

	fsdrv = f->fsdev->driver;

	if (fsdrv != ramfs_driver)
		assert_command_context();

	if (fsdrv->discard_range)
		ret = fsdrv->discard_range(&f->fsdev->dev, f, count, offset);
	else
		ret = -ENOSYS;

	return errno_set(ret);
}

int protect_file(const char *file, int prot)
{
	int fd, ret;

	fd = open(file, O_WRONLY);
	if (fd < 0)
		return fd;

	ret = protect(fd, ~0, 0, prot);

	close(fd);

	return ret;
}

void *memmap(int fd, int flags)
{
	struct fs_driver *fsdrv;
	FILE *f = fd_to_file(fd, false);
	void *retp = MAP_FAILED;
	int ret;

	if (IS_ERR(f))
		return retp;

	fsdrv = f->fsdev->driver;

	if (fsdrv != ramfs_driver)
		assert_command_context();

	if (fsdrv->memmap)
		ret = fsdrv->memmap(&f->fsdev->dev, f, &retp, flags);
	else
		ret = -EINVAL;

	errno_set(ret);
	return retp;
}
EXPORT_SYMBOL(memmap);

int close(int fd)
{
	FILE *f = fd_to_file(fd, true);
	int ret = 0;

	if (IS_ERR(f))
		return -errno;

	if (!(f->flags & O_PATH)) {
		struct fs_driver *fsdrv;

		fsdrv = f->fsdev->driver;

		if (fsdrv != ramfs_driver)
			assert_command_context();

		if (fsdrv->close)
			ret = fsdrv->close(&f->fsdev->dev, f);
	}

	put_file(f);

	return errno_set(ret);
}
EXPORT_SYMBOL(close);

static int fs_match(struct device *dev, struct driver *drv)
{
	return strcmp(dev->name, drv->name) ? -1 : 0;
}

static int fs_probe(struct device *dev)
{
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct driver *drv = dev->driver;
	struct fs_driver *fsdrv = container_of(drv, struct fs_driver, drv);
	int ret;

	ret = dev->driver->probe(dev);
	if (ret)
		return ret;

	fsdev->driver = fsdrv;

	list_add_tail(&fsdev->list, &fs_device_list);

	if (IS_ENABLED(CONFIG_FS_LEGACY) && !fsdev->sb.s_root) {
		ret = fs_init_legacy(fsdev);
		if (ret)
			return ret;
	}

	return 0;
}

static void dentry_kill(struct dentry *dentry)
{
	if (dentry->d_inode)
		iput(dentry->d_inode);

	if (!IS_ROOT(dentry))
		dput(dentry->d_parent);

	list_del(&dentry->d_child);
	free(dentry->name);
	free(dentry);
}

static int dentry_delete_subtree(struct super_block *sb, struct dentry *parent)
{
	struct dentry *dentry, *tmp;

	if (!parent)
		return 0;

	list_for_each_entry_safe(dentry, tmp, &parent->d_subdirs, d_child)
		dentry_delete_subtree(sb, dentry);

	dentry_kill(parent);

	return 0;
}

static void destroy_inode(struct inode *inode)
{
	if (inode->i_sb->s_op->destroy_inode)
		inode->i_sb->s_op->destroy_inode(inode);
	else
		free(inode);
}

static void fs_remove(struct device *dev)
{
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct super_block *sb = &fsdev->sb;
	struct inode *inode, *tmp;
	struct path path;
	int ret;

	if (fsdev->dev.driver) {
		dev->driver->remove(dev);
		list_del(&fsdev->list);
	}

	free(fsdev->path);
	free(fsdev->options);

	if (fsdev->cdev)
		cdev_close(fsdev->cdev);

	if (fsdev->loop && fsdev->cdev) {
		cdev_remove_loop(fsdev->cdev);

		ret = filename_lookup(AT_FDCWD, getname(fsdev->backingstore),
				      LOOKUP_FOLLOW, &path);
		if (!ret) {
			mntput(path.mnt);
			path_put(&path);
		}
	}

	if (fsdev->vfsmount.mountpoint)
		fsdev->vfsmount.mountpoint->d_flags &= ~DCACHE_MOUNTED;

	dentry_delete_subtree(sb, sb->s_root);

	list_for_each_entry_safe(inode, tmp, &sb->s_inodes, i_sb_list)
		destroy_inode(inode);

	mntput(fsdev->vfsmount.parent);

	free(fsdev->backingstore);
}

struct bus_type fs_bus = {
	.name = "fs",
	.match = fs_match,
	.probe = fs_probe,
	.remove = fs_remove,
};

static int fs_bus_init(void)
{
	return bus_register(&fs_bus);
}
pure_initcall(fs_bus_init);

int register_fs_driver(struct fs_driver *fsdrv)
{
	fsdrv->drv.bus = &fs_bus;
	register_driver(&fsdrv->drv);

	if (!strcmp(fsdrv->drv.name, "ramfs"))
		ramfs_driver = fsdrv;

	return 0;
}
EXPORT_SYMBOL(register_fs_driver);

const char *fs_detect(const char *filename, const char *fsoptions)
{
	enum filetype type;
	struct driver *drv;
	struct fs_driver *fdrv;
	bool loop = false;
	unsigned long long offset = 0;
	int ret;

	parseopt_b(fsoptions, "loop", &loop);
	parseopt_llu_suffix(fsoptions, "offset", &offset);
	if (loop)
		ret = file_name_detect_type_offset(filename, offset, &type);
	else
		ret = cdev_detect_type(filename, &type);

	if (ret || type == filetype_unknown)
		return NULL;

	bus_for_each_driver(&fs_bus, drv) {
		fdrv = drv_to_fs_driver(drv);

		if (type == fdrv->type)
			return drv->name;
	}

	return NULL;
}

int fsdev_open_cdev(struct fs_device *fsdev)
{
	unsigned long long offset = 0;
	struct path path = {};
	int ret;

	parseopt_b(fsdev->options, "loop", &fsdev->loop);
	parseopt_llu_suffix(fsdev->options, "offset", &offset);
	if (fsdev->loop) {
		ret = filename_lookup(AT_FDCWD, getname(fsdev->backingstore),
				      LOOKUP_FOLLOW, &path);
		if (ret)
			return ret;

		fsdev->cdev = cdev_create_loop(fsdev->backingstore, O_RDWR, offset);
	} else {
		fsdev->cdev = cdev_open_by_name(fsdev->backingstore, O_RDWR);
	}
	if (!fsdev->cdev) {
		path_put(&path);
		return -EINVAL;
	}

	if (path.mnt) {
		mntget(path.mnt);
		path_put(&path);
	}

	fsdev->dev.parent = fsdev->cdev->dev;

	return 0;
}

static void init_super(struct super_block *sb)
{
	INIT_LIST_HEAD(&sb->s_inodes);
}

static int fsdev_umount(struct fs_device *fsdev)
{
	int ret;

	if (fsdev->vfsmount.ref)
		return -EBUSY;

	ret = unregister_device(&fsdev->dev);
	if (ret)
		return ret;

	free(fsdev);

	return 0;
}

/**
 * umount_by_cdev Use a cdev struct to umount all mounted filesystems
 * @param cdev cdev to the according device
 * @return 0 on success or if cdev was not mounted, -errno otherwise
 */
int umount_by_cdev(struct cdev *cdev)
{
	struct fs_device *fs;
	struct fs_device *fs_tmp;
	int first_error = 0;

	for_each_fs_device_safe(fs_tmp, fs) {
		int ret;

		if (fs->cdev == cdev) {
			ret = fsdev_umount(fs);
			if (ret) {
				pr_err("Failed umounting %s, %d, continuing anyway\n",
				       fs->path, ret);
				if (!first_error)
					first_error = ret;
			}
		}
	}

	return first_error;
}
EXPORT_SYMBOL(umount_by_cdev);

struct readdir_entry {
	struct dirent d;
	struct list_head list;
};

struct readdir_callback {
	struct dir_context ctx;
	DIR *dir;
};

static int fillonedir(struct dir_context *ctx, const char *name, int namlen,
		      loff_t offset, u64 ino, unsigned int d_type)
{
	struct readdir_callback *rd = container_of(ctx, struct readdir_callback, ctx);
	struct readdir_entry *entry;

	entry = xzalloc(sizeof(*entry));
	memcpy(entry->d.d_name, name, namlen);
	list_add_tail(&entry->list, &rd->dir->entries);

	return 0;
}

int unreaddir(DIR *dir, const struct dirent *d)
{
	struct readdir_entry *entry;

	if (d != &dir->d)
		return -EINVAL;

	entry = xzalloc(sizeof(*entry));
	entry->d = *d;
	list_add(&entry->list, &dir->entries);

	return 0;
}
EXPORT_SYMBOL(unreaddir);

struct dirent *readdir(DIR *dir)
{
	struct readdir_entry *entry;

	if (!dir)
		return NULL;

	if (list_empty(&dir->entries))
		return NULL;

	entry = list_first_entry(&dir->entries, struct readdir_entry, list);

	list_del(&entry->list);
	strcpy(dir->d.d_name, entry->d.d_name);
	free(entry);

	return &dir->d;
}
EXPORT_SYMBOL(readdir);

static void stat_inode(struct inode *inode, struct stat *s)
{
	s->st_ino = inode->i_ino;
	s->st_mode = inode->i_mode;
	s->st_uid = inode->i_uid;
	s->st_gid = inode->i_gid;
	s->st_size = inode->i_size;
}

int fstat(int fd, struct stat *s)
{
	FILE *f = fd_to_file(fd, true);

	if (IS_ERR(f))
		return -errno;

	stat_inode(f->f_inode, s);

	return 0;
}
EXPORT_SYMBOL(fstat);

/*
 * cdev_get_mount_path - return the path a cdev is mounted on
 *
 * If a cdev is mounted return the path it's mounted on, NULL
 * otherwise.
 */
const char *cdev_get_mount_path(struct cdev *cdev)
{
	struct fs_device *fsdev;

	for_each_fs_device(fsdev) {
		if (fsdev->cdev && fsdev->cdev == cdev)
			return fsdev->path;
	}

	return NULL;
}

/*
 * cdev_mount_default - mount a cdev to the default path
 *
 * If a cdev is already mounted to the default mount path return the path
 * it's mounted on. If it is mounted to any other path return EBUSY.
 * Otherwise mount it to /mnt/<cdevname> and return the path. Returns an
 * error pointer on failure.
 */
const char *cdev_mount_default(struct cdev *cdev, const char *fsoptions)
{
	const char *path;
	char *newpath, *devpath;
	int ret;

	/*
	 * If this cdev is already mounted somewhere other than the
	 * default mount path return -EBUSY instead of mounting it
	 * again to avoid corruption on the filesystem. Note this
	 * ignores eventual fsoptions though. If the cdev is already
	 * mounted on the default path just return that path.
	 */
	path = cdev_get_mount_path(cdev);
	newpath = basprintf("/mnt/%s", cdev->name);

	if (path) {
		if (strcmp(newpath, path)) {
			free(newpath);
			return ERR_PTR(-EBUSY);
		} else {
			return path;
		}
	}

	make_directory(newpath);

	devpath = basprintf("/dev/%s", cdev->name);

	ret = mount(devpath, NULL, newpath, fsoptions);

	free(devpath);

	if (ret) {
		free(newpath);
		return ERR_PTR(ret);
	}

	return cdev_get_mount_path(cdev);
}

/*
 * cdev_mount - return existing mount or mount a cdev to the default path
 *
 * If a cdev is already mounted anywhere return the path
 * it's mounted on.
 * Otherwise mount it to /mnt/<cdevname> and return the path. Returns an
 * error pointer on failure.
 */
const char *cdev_mount(struct cdev *cdev)
{
	const char *path;

	path = cdev_get_mount_path(cdev);
	if (path)
		return path;

	return cdev_mount_default(cdev, NULL);
}

/*
 * mount_all - iterate over block devices and mount all devices we are able to
 */
void mount_all(void)
{
	struct device *dev;
	struct block_device *bdev;

	if (!IS_ENABLED(CONFIG_BLOCK))
		return;

	for_each_device(dev)
		device_detect(dev);

	for_each_block_device(bdev) {
		struct cdev *cdev;

		list_for_each_entry(cdev, &bdev->dev->cdevs, devices_list)
			cdev_mount_default(cdev, NULL);
	}
}

void fsdev_set_linux_rootarg(struct fs_device *fsdev, const char *str)
{
	fsdev->linux_rootarg = xstrdup(str);

	dev_add_param_fixed(&fsdev->dev, "linux.bootargs", fsdev->linux_rootarg);
}

/**
 * path_get_linux_rootarg() - Given a path return a suitable root= option for
 *                            Linux
 * @path: The path
 *
 * Return: A string containing the root= option or an ERR_PTR. the returned
 *         string must be freed by the caller.
 */
char *path_get_linux_rootarg(const char *path)
{
	struct fs_device *fsdev;
	const char *str;

	fsdev = get_fsdevice_by_path(AT_FDCWD, path);
	if (!fsdev)
		return ERR_PTR(-EINVAL);

	str = dev_get_param(&fsdev->dev, "linux.bootargs");
	if (!str)
		return ERR_PTR(-ENOSYS);

	return xstrdup(str);
}

/**
 * __is_tftp_fs() - return true when path is mounted on TFTP
 * @path: The path
 *
 * Do not use directly, use is_tftp_fs instead.
 *
 * Return: true when @path is on TFTP, false otherwise
 */
bool __is_tftp_fs(const char *path)
{
	struct fs_device *fsdev;

	fsdev = get_fsdevice_by_path(AT_FDCWD, path);
	if (!fsdev)
		return false;

	if (strcmp(fsdev->driver->drv.name, "tftp"))
		return false;

	return true;
}

/* inode.c */
unsigned int get_next_ino(void)
{
	static unsigned int ino;

	return ++ino;
}

void drop_nlink(struct inode *inode)
{
	WARN_ON(inode->i_nlink == 0);
	inode->__i_nlink--;
}

void inc_nlink(struct inode *inode)
{
	inode->__i_nlink++;
}

void clear_nlink(struct inode *inode)
{
	if (inode->i_nlink) {
		inode->__i_nlink = 0;
	}
}

void set_nlink(struct inode *inode, unsigned int nlink)
{
	if (!nlink) {
		clear_nlink(inode);
	} else {
		inode->__i_nlink = nlink;
	}
}

static struct inode *alloc_inode(struct super_block *sb)
{
	static const struct inode_operations empty_iops;
	static const struct file_operations no_open_fops;
	struct inode *inode;

	if (sb->s_op->alloc_inode)
		inode = sb->s_op->alloc_inode(sb);
	else
		inode = xzalloc(sizeof(*inode));

	inode->i_op = &empty_iops;
	inode->i_fop = &no_open_fops;
	inode->__i_nlink = 1;
	inode->i_count = 1;

	return inode;
}

struct inode *new_inode(struct super_block *sb)
{
	struct inode *inode;

	inode = alloc_inode(sb);
	if (!inode)
		return NULL;

	inode->i_sb = sb;

	list_add(&inode->i_sb_list, &sb->s_inodes);

	return inode;
}

struct inode *iget_locked(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;

	list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
		if (inode->i_ino == ino)
			return iget(inode);
	}

	inode = new_inode(sb);
	if (!inode)
		return NULL;

	inode->i_state = I_NEW;
	inode->i_ino = ino;

	return inode;
}

void iget_failed(struct inode *inode)
{
	iput(inode);
}

void iput(struct inode *inode)
{
	if (!inode)
		return;

	inode->i_count--;

	if (!inode->i_count) {
		list_del(&inode->i_sb_list);
		destroy_inode(inode);
	}
}

struct inode *iget(struct inode *inode)
{
	inode->i_count++;

	return inode;
}

/* dcache.c */

/*
 * refcounting is implemented but right now we do not do anything with
 * the refcounting information. Dentries are never freed unless the
 * filesystem they are on is unmounted. In this case we do not care
 * about the refcounts so we may free up a dentry that is actually used
 * (file is opened). This leaves room for improvements.
 */
void dput(struct dentry *dentry)
{
	if (!dentry)
		return;

	if (!dentry->d_count)
		return;

	dentry->d_count--;
}

struct dentry *dget(struct dentry *dentry)
{
	if (!dentry)
		return NULL;

	dentry->d_count++;

	return dentry;
}

const struct qstr slash_name = QSTR_INIT("/", 1);

void d_set_d_op(struct dentry *dentry, const struct dentry_operations *op)
{
	dentry->d_op = op;

	if (!op)
		return;

	if (op->d_revalidate)
		dentry->d_flags |= DCACHE_OP_REVALIDATE;
}

/**
 * __d_alloc	-	allocate a dcache entry
 * @sb: filesystem it will belong to
 * @name: qstr of the name
 *
 * Allocates a dentry. It returns %NULL if there is insufficient memory
 * available. On a success the dentry is returned. The name passed in is
 * copied and the copy passed in may be reused after this call.
 */
static struct dentry *__d_alloc(struct super_block *sb, const struct qstr *name)
{
	struct dentry *dentry;

	dentry = xzalloc(sizeof(*dentry));
	if (!dentry)
		return NULL;

	if (!name)
		name = &slash_name;

	dentry->name = malloc(name->len + 1);
	if (!dentry->name)
		return NULL;

	memcpy(dentry->name, name->name, name->len);
	dentry->name[name->len] = 0;

	dentry->d_name.len = name->len;
	dentry->d_name.name = dentry->name;

	dentry->d_count = 1;
	dentry->d_parent = dentry;
	dentry->d_sb = sb;
	INIT_LIST_HEAD(&dentry->d_subdirs);
	INIT_LIST_HEAD(&dentry->d_child);
	d_set_d_op(dentry, dentry->d_sb->s_d_op);

	return dentry;
}

/**
 * d_alloc	-	allocate a dcache entry
 * @parent: parent of entry to allocate
 * @name: qstr of the name
 *
 * Allocates a dentry. It returns %NULL if there is insufficient memory
 * available. On a success the dentry is returned. The name passed in is
 * copied and the copy passed in may be reused after this call.
 */
static struct dentry *d_alloc(struct dentry *parent, const struct qstr *name)
{
	struct dentry *dentry = __d_alloc(parent->d_sb, name);
	if (!dentry)
		return NULL;

	dget(parent);

	dentry->d_parent = parent;
	list_add(&dentry->d_child, &parent->d_subdirs);

	return dentry;
}

struct dentry *d_alloc_anon(struct super_block *sb)
{
	return __d_alloc(sb, NULL);
}

static unsigned d_flags_for_inode(struct inode *inode)
{
        if (!inode)
                return DCACHE_MISS_TYPE;

	if (S_ISDIR(inode->i_mode))
		return DCACHE_DIRECTORY_TYPE;

	if (inode->i_op->get_link)
		return DCACHE_SYMLINK_TYPE;

	return DCACHE_REGULAR_TYPE;
}

void d_instantiate(struct dentry *dentry, struct inode *inode)
{
	dentry->d_inode = inode;
	dentry->d_flags &= ~DCACHE_ENTRY_TYPE;
	dentry->d_flags |= d_flags_for_inode(inode);
}

struct dentry *d_make_root(struct inode *inode)
{
	struct dentry *res;

	if (!inode)
		return NULL;

	res = d_alloc_anon(inode->i_sb);
	if (!res)
		return NULL;

	d_instantiate(res, inode);

	return res;
}

void d_add(struct dentry *dentry, struct inode *inode)
{
	dentry->d_inode = inode;
	dentry->d_flags &= ~DCACHE_ENTRY_TYPE;
	dentry->d_flags |= d_flags_for_inode(inode);
}

static bool d_same_name(const struct dentry *dentry,
			const struct dentry *parent,
			const struct qstr *name)
{
	if (dentry->d_name.len != name->len)
		return false;

	return strncmp(dentry->d_name.name, name->name, name->len) == 0;
}

static struct dentry *d_lookup(const struct dentry *parent, const struct qstr *name)
{
	struct dentry *dentry;

	list_for_each_entry(dentry, &parent->d_subdirs, d_child) {
		if (!d_same_name(dentry, parent, name))
			continue;

		dget(dentry);

		return dentry;
	}

	return NULL;
}

static void d_invalidate(struct dentry *dentry)
{
}

static int d_no_revalidate(struct dentry *dir, unsigned int flags)
{
	return 0;
}

const struct dentry_operations no_revalidate_d_ops = {
	.d_revalidate = d_no_revalidate,
};

static inline int d_revalidate(struct dentry *dentry, unsigned int flags)
{
	if (unlikely(dentry->d_flags & DCACHE_OP_REVALIDATE))
		return dentry->d_op->d_revalidate(dentry, flags);
	else
		return 1;
}

/*
 * This looks up the name in dcache and possibly revalidates the found dentry.
 * NULL is returned if the dentry does not exist in the cache.
 */
static struct dentry *lookup_dcache(const struct qstr *name,
				    struct dentry *dir,
				    unsigned int flags)
{
	struct dentry *dentry = d_lookup(dir, name);
	if (dentry) {
		int error = d_revalidate(dentry, flags);
		if (unlikely(error <= 0)) {
			if (!error)
				d_invalidate(dentry);
			dput(dentry);
			return ERR_PTR(error);
		}
	}
	return dentry;
}

static inline void __d_clear_type_and_inode(struct dentry *dentry)
{
	dentry->d_flags &= ~(DCACHE_ENTRY_TYPE | DCACHE_FALLTHRU);

	dentry->d_inode = NULL;
}

/*
 * Release the dentry's inode, using the filesystem
 * d_iput() operation if defined.
 */
static void dentry_unlink_inode(struct dentry * dentry)
{
	struct inode *inode = dentry->d_inode;

	__d_clear_type_and_inode(dentry);
	iput(inode);
}

void d_delete(struct dentry * dentry)
{
	dentry_unlink_inode(dentry);
}

/*
 * These are the Linux name resolve functions from fs/namei.c
 *
 * The implementation is more or less directly ported from the
 * Linux Kernel (as of Linux-4.16) minus the RCU and locking code.
 */

enum {WALK_FOLLOW = 1, WALK_MORE = 2};

/*
 * Define EMBEDDED_LEVELS to MAXSYMLINKS so we do not have to
 * dynamically allocate a path stack.
 */
#define EMBEDDED_LEVELS MAXSYMLINKS

struct nameidata {
	struct path	path;
	struct qstr	last;
	unsigned int	flags;
	int		last_type;
	unsigned	depth;
	int		total_link_count;
	struct saved {
		struct path link;
		const char *name;
	} *stack, internal[EMBEDDED_LEVELS];
	struct filename	*name;
	struct inode	*link_inode;
	struct dentry   *d_root;
};

struct filename {
	char *name;
	int refcnt;
};

static void set_nameidata(struct nameidata *p, struct filename *name)
{
	p->stack = p->internal;
	p->name = name;
	p->total_link_count = 0;
	p->d_root = d_root;
}

static void path_get(const struct path *path)
{
	mntget(path->mnt);
	dget(path->dentry);
}

static void path_put(const struct path *path)
{
	dput(path->dentry);
	mntput(path->mnt);
}

static inline void get_root(struct path *root)
{
	root->dentry = d_root;
	root->mnt = mnt_root;

	path_get(root);
}

static inline void get_pwd(struct path *pwd)
{
	pwd->dentry = cwd_dentry;
	pwd->mnt = cwd_mnt;

	path_get(pwd);
}

static inline void put_link(struct nameidata *nd)
{
	struct saved *last = nd->stack + --nd->depth;
	path_put(&last->link);
}

static int automount_mount(struct dentry *dentry);

static void path_put_conditional(struct path *path, struct nameidata *nd)
{
	dput(path->dentry);
	if (path->mnt != nd->path.mnt)
		mntput(path->mnt);
}

static int follow_automount(struct path *path, struct nameidata *nd,
			    bool *need_mntput)
{
	/* We don't want to mount if someone's just doing a stat -
	 * unless they're stat'ing a directory and appended a '/' to
	 * the name.
	 *
	 * We do, however, want to mount if someone wants to open or
	 * create a file of any type under the mountpoint, wants to
	 * traverse through the mountpoint or wants to open the
	 * mounted directory.  Also, autofs may mark negative dentries
	 * as being automount points.  These will need the attentions
	 * of the daemon to instantiate them before they can be used.
	 */
	if (!(nd->flags & (LOOKUP_PARENT | LOOKUP_DIRECTORY)) && path->dentry->d_inode)
		return -EISDIR;

	return automount_mount(path->dentry);
}

/*
 * Handle a dentry that is managed in some way.
 * - Flagged for transit management (autofs)
 * - Flagged as mountpoint
 * - Flagged as automount point
 *
 * This may only be called in refwalk mode.
 *
 * Serialization is taken care of in namespace.c
 */
static int follow_managed(struct path *path, struct nameidata *nd)
{
	struct vfsmount *mnt = path->mnt;
	unsigned managed = path->dentry->d_flags;
	bool need_mntput = false;
	int ret = 0;

	while (managed = path->dentry->d_flags,
		managed &= DCACHE_MANAGED_DENTRY,
		managed != 0) {

		if (managed & DCACHE_MOUNTED) {
			struct vfsmount *mounted = lookup_mnt(path);

			if (mounted) {
				dput(path->dentry);
				if (need_mntput)
					mntput(path->mnt);
				path->mnt = mounted;
				path->dentry = dget(mounted->mnt_root);
				need_mntput = true;
				continue;
			}
		}

		/* Handle an automount point */
		if (managed & DCACHE_NEED_AUTOMOUNT) {
			ret = follow_automount(path, nd, &need_mntput);
			if (ret < 0)
				break;
			continue;
		}

		/* We didn't change the current path point */
		break;
	}

	if (need_mntput && path->mnt == mnt)
		mntput(path->mnt);
	if (ret == -EISDIR || !ret)
		ret = 1;
	if (need_mntput)
		nd->flags |= LOOKUP_JUMPED;
	if (ret < 0)
		path_put_conditional(path, nd);
	return ret;
}

static struct dentry *__lookup_hash(const struct qstr *name,
		struct dentry *base, unsigned int flags)
{
	struct dentry *dentry;
	struct dentry *old;
	struct inode *dir;

	if (!base)
		return ERR_PTR(-ENOENT);

	dentry = lookup_dcache(name, base, flags);
	if (dentry)
		return dentry;

	dentry = d_alloc(base, name);
	if (unlikely(!dentry))
		return ERR_PTR(-ENOMEM);

	dir = base->d_inode;
	old = dir->i_op->lookup(dir, dentry, flags);
	if (IS_ERR(old)) {
		dput(dentry);
		return old;
	}

	if (unlikely(old)) {
		dput(dentry);
		dentry = old;
	}

	return dentry;
}

static int lookup_fast(struct nameidata *nd, struct path *path)
{
	struct dentry *dentry, *parent = nd->path.dentry;

	dentry = __lookup_hash(&nd->last, parent, 0);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	if (d_is_negative(dentry)) {
		dput(dentry);
		return -ENOENT;
	}

	path->dentry = dentry;
	path->mnt = nd->path.mnt;

	return follow_managed(path, nd);
}

/*
 * follow_up - Find the mountpoint of path's vfsmount
 *
 * Given a path, find the mountpoint of its source file system.
 * Replace @path with the path of the mountpoint in the parent mount.
 * Up is towards /.
 *
 * Return 1 if we went up a level and 0 if we were already at the
 * root.
 */
static int follow_up(struct nameidata *nd)
{
	struct path *path = &nd->path;
	struct vfsmount *mnt = path->mnt;
	struct dentry *mountpoint;

	if (nd->d_root == path->dentry)
		return 0;

	mntget(mnt->parent);
	mountpoint = dget(mnt->mountpoint);
	dput(path->dentry);
	path->dentry = mountpoint;
	mntput(path->mnt);
	path->mnt = mnt->parent;

	return 1;
}

static void follow_mount(struct path *path)
{
	while (d_mountpoint(path->dentry)) {
		struct vfsmount *mounted = lookup_mnt(path);
		if (!mounted)
			break;
		dput(path->dentry);
		path->mnt = mounted;
		path->dentry = dget(mounted->mnt_root);
	}
}

static int path_parent_directory(struct path *path)
{
	struct dentry *old = path->dentry;

	path->dentry = dget(path->dentry->d_parent);
	dput(old);

	return 0;
}

static int follow_dotdot(struct nameidata *nd)
{
	while (1) {
		if (nd->path.dentry != nd->path.mnt->mnt_root) {
			int ret = path_parent_directory(&nd->path);
			if (ret)
				return ret;
			break;
		}

		if (!follow_up(nd))
			break;
	}

	follow_mount(&nd->path);

	return 0;
}

static inline int handle_dots(struct nameidata *nd, int type)
{
	if (type == LAST_DOTDOT) {
		return follow_dotdot(nd);
	}
	return 0;
}

static inline void path_to_nameidata(const struct path *path,
					struct nameidata *nd)
{
	dput(nd->path.dentry);
	if (nd->path.mnt != path->mnt)
		mntput(nd->path.mnt);
	nd->path.mnt = path->mnt;
	nd->path.dentry = path->dentry;
}

static const char *get_link(struct nameidata *nd)
{
	struct saved *last = nd->stack + nd->depth - 1;
	struct dentry *dentry = last->link.dentry;
	struct inode *inode = nd->link_inode;
	const char *res;

	nd->last_type = LAST_BIND;
	res = inode->i_link;
	if (!res) {
		res = inode->i_op->get_link(dentry, inode);
		if (IS_ERR_OR_NULL(res))
			return res;
	}
	if (*res == '/') {
		while (unlikely(*++res == '/'))
			;
	}
	if (!*res)
		res = NULL;
	return res;
}

static int pick_link(struct nameidata *nd, struct path *link,
		     struct inode *inode)
{
	struct saved *last;

	if (unlikely(nd->total_link_count++ >= MAXSYMLINKS)) {
		path_to_nameidata(link, nd);
		return -ELOOP;
	}

	if (link->mnt == nd->path.mnt)
		mntget(link->mnt);

	last = nd->stack + nd->depth++;
	last->link = *link;
	nd->link_inode = inode;

	return 1;
}

/*
 * Do we need to follow links? We _really_ want to be able
 * to do this check without having to look at inode->i_op,
 * so we keep a cache of "no, this doesn't need follow_link"
 * for the common case.
 */
static inline int step_into(struct nameidata *nd, struct path *path,
			    int flags, struct inode *inode)
{
	if (!(flags & WALK_MORE) && nd->depth)
		put_link(nd);

	if (likely(!d_is_symlink(path->dentry)) ||
	   !(flags & WALK_FOLLOW || nd->flags & LOOKUP_FOLLOW)) {
		/* not a symlink or should not follow */
		path_to_nameidata(path, nd);
		return 0;
	}

	return pick_link(nd, path, inode);
}

static int walk_component(struct nameidata *nd, int flags)
{
	struct path path = {};
	int err;

	/*
	 * "." and ".." are special - ".." especially so because it has
	 * to be able to know about the current root directory and
	 * parent relationships.
	 */
	if (nd->last_type != LAST_NORM) {
		err = handle_dots(nd, nd->last_type);
		if (!(flags & WALK_MORE) && nd->depth)
			put_link(nd);
		return err;
	}

	err = lookup_fast(nd, &path);
	if (err < 0)
		return err;

	if (err == 0 && d_is_negative(path.dentry)) {
		path_to_nameidata(&path, nd);
		return -ENOENT;
	}

	return step_into(nd, &path, flags, d_inode(path.dentry));
}

static int component_len(const char *name, char separator)
{
	int len = 0;

	while (name[len] && name[len] != separator)
		len++;

	return len;
}

static struct filename *getname(const char *filename)
{
	struct filename *result;

	if (!*filename)
		return ERR_PTR(-ENOENT);

	result = malloc(sizeof(*result));
	if (!result)
		return ERR_PTR(-ENOMEM);

	result->name = strdup(filename);
	if (!result->name) {
		free(result);
		return ERR_PTR(-ENOMEM);
	}

	result->refcnt = 1;

	return result;
}

static void putname(struct filename *name)
{
	BUG_ON(name->refcnt <= 0);

	if (--name->refcnt > 0)
		return;

	free(name->name);
	free(name);
}

static struct fs_device *get_fsdevice_by_dentry(struct dentry *dentry)
{
	struct super_block *sb;

	sb = dentry->d_sb;

	return container_of(sb, struct fs_device, sb);
}

static bool dentry_is_tftp(struct dentry *dentry)
{
	struct fs_device *fsdev;

	fsdev = get_fsdevice_by_dentry(dentry);
	if (!fsdev)
		return false;

	if (strcmp(fsdev->driver->drv.name, "tftp"))
		return false;

	return true;
}

/*
 * Name resolution.
 * This is the basic name resolution function, turning a pathname into
 * the final dentry. We expect 'base' to be positive and a directory.
 *
 * Returns 0 and nd will have valid dentry and mnt on success.
 * Returns error and drops reference to input namei data on failure.
 */
static int link_path_walk(const char *name, struct nameidata *nd)
{
	int err;
	char separator = '/';

	if (!*name)
		return -ENOENT;
	while (*name=='/')
		name++;
	if (!*name)
		return 0;

	/* At this point we know we have a real path component. */
	for(;;) {
		int len;
		int type;

		len = component_len(name, separator);

		type = LAST_NORM;
		if (name[0] == '.') switch (len) {
			case 2:
				if (name[1] == '.') {
					type = LAST_DOTDOT;
					nd->flags |= LOOKUP_JUMPED;
				}
				break;
			case 1:
				type = LAST_DOT;
		}
		if (likely(type == LAST_NORM))
			nd->flags &= ~LOOKUP_JUMPED;

		nd->last.len = len;
		nd->last.name = name;
		nd->last_type = type;

		name += len;
		if (!*name)
			goto OK;

		/*
		 * If it wasn't NUL, we know it was '/'. Skip that
		 * slash, and continue until no more slashes.
		 */
		do {
			name++;
		} while (unlikely(*name == separator));

		if (unlikely(!*name)) {
OK:
			/* pathname body, done */
			if (!nd->depth)
				return 0;
			name = nd->stack[nd->depth - 1].name;
			/* trailing symlink, done */
			if (!name)
				return 0;
			/* last component of nested symlink */
			err = walk_component(nd, WALK_FOLLOW);
		} else {
			/* not the last component */
			err = walk_component(nd, WALK_FOLLOW | WALK_MORE);
		}

		if (err < 0)
			return err;

		/*
		 * barebox specific hack for TFTP. TFTP does not support
		 * looking up directories, only the files in directories.
		 * Since the filename is not known at this point we replace
		 * the path separator with an invalid char so that TFTP will
		 * get the full remaining path including slashes.
		 */
		if (dentry_is_tftp(nd->path.dentry))
			separator = 0x1;

		if (err) {
			const char *s = get_link(nd);

			if (IS_ERR(s))
				return PTR_ERR(s);
			err = 0;
			if (unlikely(!s)) {
				/* jumped */
				put_link(nd);
			} else {
				nd->stack[nd->depth - 1].name = name;
				name = s;
				continue;
			}
		}
		if (unlikely(!d_can_lookup(nd->path.dentry)))
			return -ENOTDIR;
	}
}

static const char *path_init(int dirfd, struct nameidata *nd, unsigned flags)
{
	const char *s = nd->name->name;
	FILE *f = NULL;

	nd->last_type = LAST_ROOT; /* if there are only slashes... */
	nd->flags = flags | LOOKUP_JUMPED | LOOKUP_PARENT;
	nd->depth = 0;

	nd->path.mnt = NULL;
	nd->path.dentry = NULL;

	/* We don't check for error here yet, as POSIX allows checking
	 * whether paths are absolute with openat(-1, path, O_PATH)
	 */
	if (dirfd != AT_FDCWD)
		f = fd_to_file(dirfd, true);

	if (*s == '/') {
		get_root(&nd->path);
	} else if (dirfd == AT_FDCWD) {
		get_pwd(&nd->path);
	} else {
		if (IS_ERR(f))
			return ERR_CAST(f);

		nd->path.mnt = &f->fsdev->vfsmount;
		nd->path.dentry = f->dentry;
		follow_mount(&nd->path);
	}

	return s;
}

static const char *trailing_symlink(struct nameidata *nd)
{
	const char *s;

	nd->flags |= LOOKUP_PARENT;
	nd->stack[0].name = NULL;
	s = get_link(nd);

	return s ? s : "";
}

static inline int lookup_last(struct nameidata *nd)
{
	if (nd->last_type == LAST_NORM && nd->last.name[nd->last.len])
		nd->flags |= LOOKUP_FOLLOW | LOOKUP_DIRECTORY;

	nd->flags &= ~LOOKUP_PARENT;
	return walk_component(nd, 0);
}

static void terminate_walk(struct nameidata *nd)
{
	int i;

	path_put(&nd->path);
	for (i = 0; i < nd->depth; i++)
		path_put(&nd->stack[i].link);

	nd->depth = 0;
}

/* Returns 0 and nd will be valid on success; Retuns error, otherwise. */
static int path_parentat(int dirfd, struct nameidata *nd, unsigned flags,
				struct path *parent)
{
	const char *s = path_init(dirfd, nd, flags);
	int err;

	if (IS_ERR(s))
		return PTR_ERR(s);

	err = link_path_walk(s, nd);
	if (!err) {
		*parent = nd->path;
		nd->path.mnt = NULL;
		nd->path.dentry = NULL;
	}
	terminate_walk(nd);
	return err;
}

static struct filename *filename_parentat(int dirfd,
				struct filename *name,
				unsigned int flags, struct path *parent,
				struct qstr *last, int *type)
{
	int retval;
	struct nameidata nd;

	if (IS_ERR(name))
		return name;

	set_nameidata(&nd, name);

	retval = path_parentat(dirfd, &nd, flags, parent);
	if (likely(!retval)) {
		*last = nd.last;
		*type = nd.last_type;
	} else {
		putname(name);
		name = ERR_PTR(retval);
	}

	return name;
}

static struct dentry *filename_create(int dirfd, struct filename *name,
				struct path *path, unsigned int lookup_flags)
{
	struct dentry *dentry = ERR_PTR(-EEXIST);
	struct qstr last;
	int type;
	int error;
	bool is_dir = (lookup_flags & LOOKUP_DIRECTORY);

	name = filename_parentat(dirfd, name, 0, path, &last, &type);
	if (IS_ERR(name))
		return ERR_CAST(name);

	/*
	 * Yucky last component or no last component at all?
	 * (foo/., foo/.., /////)
	 */
	if (unlikely(type != LAST_NORM))
		goto out;

	/*
	 * Do the final lookup.
	 */
	dentry = __lookup_hash(&last, path->dentry, lookup_flags);
	if (IS_ERR(dentry))
		goto unlock;

	error = -EEXIST;
	if (d_is_positive(dentry))
		goto fail;

	/*
	 * Special case - lookup gave negative, but... we had foo/bar/
	 * From the vfs_mknod() POV we just have a negative dentry -
	 * all is fine. Let's be bastards - you had / on the end, you've
	 * been asking for (non-existent) directory. -ENOENT for you.
	 */
	if (unlikely(!is_dir && last.name[last.len])) {
		error = -ENOENT;
		goto fail;
	}
	putname(name);
	return dentry;
fail:
	dput(dentry);
	dentry = ERR_PTR(error);
unlock:
out:
	path_put(path);
	putname(name);
	return dentry;
}

static int filename_lookup(int dirfd, struct filename *name, unsigned flags,
			   struct path *path)
{
	int err;
	struct nameidata nd;
	const char *s;

	if (IS_ERR(name))
		return PTR_ERR(name);

	set_nameidata(&nd, name);

	s = path_init(dirfd, &nd, flags);
	if (IS_ERR(s))
		return PTR_ERR(s);

	while (!(err = link_path_walk(s, &nd)) && ((err = lookup_last(&nd)) > 0)) {
		s = trailing_symlink(&nd);
		if (IS_ERR(s)) {
			err = PTR_ERR(s);
			break;
		}
	}

	if (!err && nd.flags & LOOKUP_DIRECTORY)
		if (!d_can_lookup(nd.path.dentry))
			err = -ENOTDIR;
	if (!err) {
		*path = nd.path;
		nd.path.mnt = NULL;
		nd.path.dentry = NULL;
	}

	terminate_walk(&nd);
	putname(name);

	return err;
}

static struct fs_device *get_fsdevice_by_path(int dirfd, const char *pathname)
{
	struct fs_device *fsdev;
	struct path path;
	int ret;

	ret = filename_lookup(dirfd, getname(pathname), 0, &path);
	if (ret)
		return NULL;

	fsdev = get_fsdevice_by_dentry(path.dentry);

	path_put(&path);

	return fsdev;
}

static int vfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	int error;

	if (!dir->i_op->rmdir)
		return -EPERM;

	dget(dentry);

	error = dir->i_op->rmdir(dir, dentry);
	if (error)
		goto out;

	dentry->d_inode->i_flags |= S_DEAD;

out:
	dput(dentry);

	if (!error)
		d_delete(dentry);

	return error;
}

static int vfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int error;

	if (!dir->i_op->mkdir)
		return -EPERM;

	mode &= (S_IRWXUGO|S_ISVTX);

	error = dir->i_op->mkdir(dir, dentry, mode);

	return error;
}

/* libfs.c */

/* ---------------------------------------------------------------- */
int mkdirat(int dirfd, const char *pathname, mode_t mode)
{
	struct dentry *dentry;
	struct path path;
	int error;
	unsigned int lookup_flags = LOOKUP_DIRECTORY;

	dentry = filename_create(dirfd, getname(pathname), &path, lookup_flags);
	if (IS_ERR(dentry)) {
		error = PTR_ERR(dentry);
		goto out;
	}

	error = vfs_mkdir(path.dentry->d_inode, dentry, mode);

	dput(dentry);
	path_put(&path);
out:
	return errno_set(error);
}
EXPORT_SYMBOL(mkdirat);

static int rmdirat(int dirfd, const char *pathname)
{
	int error = 0;
	struct filename *name;
	struct dentry *dentry;
	struct path path;
	struct qstr last;
	int type;

	name = filename_parentat(dirfd, getname(pathname), 0,
				&path, &last, &type);
	if (IS_ERR(name))
		return PTR_ERR(name);

	switch (type) {
	case LAST_DOTDOT:
		error = -ENOTEMPTY;
		goto out;
	case LAST_DOT:
		error = -EINVAL;
		goto out;
	case LAST_ROOT:
		error = -EBUSY;
		goto out;
	}

	dentry = __lookup_hash(&last, path.dentry, 0);
	if (d_is_negative(dentry)) {
		error = -ENOENT;
		goto out;
	}
	if (d_mountpoint(dentry)) {
		error = -EBUSY;
		goto out;
	}

	if (!d_is_dir(dentry)) {
		error = -ENOTDIR;
		goto out;
	}

	error = vfs_rmdir(path.dentry->d_inode, dentry);

	dput(dentry);
out:
	path_put(&path);
	putname(name);

	return errno_set(error);
}

int openat(int dirfd, const char *pathname, int flags)
{
	struct fs_device *fsdev;
	struct fs_driver *fsdrv;
	struct super_block *sb;
	FILE *f;
	int error = 0;
	struct inode *inode = NULL;
	struct dentry *dentry = NULL;
	struct nameidata nd;
	const char *s;
	struct filename *filename;

	if (flags & O_TMPFILE) {
		fsdev = get_fsdevice_by_path(dirfd, pathname);
		if (!fsdev) {
			errno = ENOENT;
			return -errno;
		}

		if (fsdev->driver != ramfs_driver) {
			errno = EOPNOTSUPP;
			return -errno;
		}

		f = get_file();
		if (!f) {
			errno = EMFILE;
			return -errno;
		}

		f->path = NULL;
		f->dentry = NULL;
		f->f_inode = new_inode(&fsdev->sb);
		f->f_inode->i_mode = S_IFREG;
		f->flags = flags;
		f->size = 0;
		f->fsdev = fsdev;

		return f->no;
	}

	filename = getname(pathname);
	if (IS_ERR(filename))
		return PTR_ERR(filename);

	set_nameidata(&nd, filename);

	s = path_init(dirfd, &nd, LOOKUP_FOLLOW);
	if (IS_ERR(s))
		return PTR_ERR(s);

	while (1) {
		error = link_path_walk(s, &nd);
		if (error)
			break;

		if (!d_is_dir(nd.path.dentry)) {
			error = -ENOTDIR;
			break;
		}

		dentry = __lookup_hash(&nd.last, nd.path.dentry, 0);
		if (IS_ERR(dentry)) {
			error = PTR_ERR(dentry);
			break;
		}

		if (!d_is_symlink(dentry))
			break;

		dput(dentry);

		error = lookup_last(&nd);
		if (error <= 0)
			break;

		s = trailing_symlink(&nd);
		if (IS_ERR(s)) {
			error = PTR_ERR(s);
			break;
		}
	}

	terminate_walk(&nd);
	putname(nd.name);

	if (error)
		goto out1;

	if (d_is_negative(dentry)) {
		if (flags & O_CREAT) {
			error = create(nd.path.dentry, dentry);
			if (error)
				goto out1;
		} else {
			dput(dentry);
			error = -ENOENT;
			goto out1;
		}
	} else if (!(flags & O_PATH)) {
		if (d_is_dir(dentry) && !dentry_is_tftp(dentry)) {
			error = -EISDIR;
			goto out1;
		}
	}

	inode = d_inode(dentry);

	f = get_file();
	if (!f) {
		error = -EMFILE;
		goto out1;
	}

	f->path = dpath(dentry, d_root);
	f->dentry = dentry;
	f->f_inode = iget(inode);
	f->flags = flags;
	f->size = inode->i_size;

	sb = inode->i_sb;
	fsdev = container_of(sb, struct fs_device, sb);
	fsdrv = fsdev->driver;

	f->fsdev = fsdev;

	if (flags & O_PATH)
		return f->no;

	if (fsdrv->open) {
		char *pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

		error = fsdrv->open(&fsdev->dev, f, pathname);
		free(pathname);
		if (error)
			goto out;
	}

	if (flags & O_TRUNC) {
		error = fsdev_truncate(&fsdev->dev, f, 0);
		f->size = 0;
		inode->i_size = 0;
		if (error)
			goto out;
	}

	if (flags & O_APPEND)
		f->pos = f->size;

	return f->no;

out:
	put_file(f);
out1:
	return errno_set(error);
}
EXPORT_SYMBOL(openat);

static const char *fd_getpath(int fd)
{
	FILE *f;

	if (fd < 0)
		return ERR_PTR(errno_set(fd));

	f = fd_to_file(fd, true);
	if (IS_ERR(f))
		return ERR_CAST(f);

	return f->path;
}

int unlinkat(int dirfd, const char *pathname, int flags)
{
	int ret;
	struct dentry *dentry;
	struct inode *inode;
	struct path path;

	if (flags == AT_REMOVEDIR)
		return rmdirat(dirfd, pathname);
	if (flags)
		return -EINVAL;

	ret = filename_lookup(dirfd, getname(pathname), 0, &path);
	if (ret)
		goto out;

	dentry = path.dentry;

	if (d_is_dir(dentry)) {
		ret = -EISDIR;
		goto out_put;
	}

	inode = d_inode(dentry->d_parent);

	if (!inode->i_op->unlink) {
		ret = -EPERM;
		goto out_put;
	}

	ret = inode->i_op->unlink(inode, dentry);
	if (ret)
		goto out_put;

	d_delete(dentry);

out_put:
	path_put(&path);
out:
	return errno_set(ret);
}
EXPORT_SYMBOL(unlinkat);

static int vfs_symlink(struct inode *dir, struct dentry *dentry, const char *oldname)
{
	if (!dir->i_op->symlink)
		return -EPERM;

	return dir->i_op->symlink(dir, dentry, oldname);
}

int symlink(const char *pathname, const char *newpath)
{
	struct dentry *dentry;
	struct path path;
	int error;
	unsigned int lookup_flags = LOOKUP_DIRECTORY;

	dentry = filename_create(AT_FDCWD, getname(newpath), &path, lookup_flags);
	if (IS_ERR(dentry)) {
		error = PTR_ERR(dentry);
		goto out;
	}

	error = vfs_symlink(path.dentry->d_inode, dentry, pathname);
out:
	return errno_set(error);
}
EXPORT_SYMBOL(symlink);

static void __release_dir(DIR *d)
{
	while (!list_empty(&d->entries)) {
		struct readdir_entry *entry =
			list_first_entry(&d->entries, struct readdir_entry, list);

		list_del(&entry->list);
		free(entry);
	}
}

static int __opendir(DIR *d)
{
	int ret;
	struct file file = {};
	struct path *path = &d->path;
	struct dentry *dir = path->dentry;
	struct readdir_callback rd = {
		.ctx = {
			.actor = fillonedir,
		},
	};

	file.f_path.dentry = dir;
	file.f_inode = d_inode(dir);
	file.f_op = dir->d_inode->i_fop;

	INIT_LIST_HEAD(&d->entries);
	rd.dir = d;

	ret = file.f_op->iterate(&file, &rd.ctx);
	if (ret)
		__release_dir(d);

	return ret;
}

DIR *opendir(const char *pathname)
{
	int ret;
	struct dentry *dir;
	struct inode *inode;
	DIR *d;
	struct path path = {};

	ret = filename_lookup(AT_FDCWD, getname(pathname),
			      LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &path);
	if (ret)
		goto out;

	dir = path.dentry;

	if (d_is_negative(dir)) {
		ret = -ENOENT;
		goto out_put;
	}

	inode = d_inode(dir);

	if (!S_ISDIR(inode->i_mode)) {
		ret = -ENOTDIR;
		goto out_put;
	}

	d = xzalloc(sizeof(*d));
	d->path = path;
	d->fd = -ENOENT;

	ret = __opendir(d);
	if (ret)
		goto out_free;

	return d;

out_free:
	free(d);
out_put:
	path_put(&path);
out:
	errno_set(ret);

	return NULL;
}
EXPORT_SYMBOL(opendir);

DIR *fdopendir(int fd)
{
	const char *path;
	DIR *dir;

	path = fd_getpath(fd);
	if (IS_ERR(path))
		return NULL;

	dir = opendir(path);
	if (!dir)
		return NULL;

	/* we intentionally don't increment the reference count,
	 * as POSIX specifies that fd ownership is transferred
	 */
	dir->fd = fd;
	return dir;
}
EXPORT_SYMBOL(fdopendir);

int closedir(DIR *dir)
{
	if (!dir)
		return errno_set(-EBADF);

	path_put(&dir->path);
	__release_dir(dir);
	if (dir->fd >= 0)
		close(dir->fd);
	free(dir);

	return 0;
}
EXPORT_SYMBOL(closedir);

int rewinddir(DIR *dir)
{
	if (!dir)
		return errno_set(-EBADF);

	__release_dir(dir);

	return __opendir(dir);
}
EXPORT_SYMBOL(rewinddir);

int readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
	int ret;
	struct dentry *dentry;
	struct inode *inode;
	const char *link;
	struct path path = {};

	ret = filename_lookup(dirfd, getname(pathname), 0, &path);
	if (ret)
		goto out;

	dentry = path.dentry;

	if (!d_is_symlink(dentry)) {
		ret = -EINVAL;
		goto out_put;
	}

	inode = d_inode(dentry);

	if (!inode->i_op->get_link) {
		ret = -EPERM;
		goto out_put;
	}

	link = inode->i_op->get_link(dentry, inode);
	if (IS_ERR(link)) {
		ret =  PTR_ERR(link);
		goto out_put;
	}

	strncpy(buf, link, bufsiz);

	ret = 0;
out_put:
	path_put(&path);
out:
	return errno_set(ret);
}
EXPORT_SYMBOL(readlinkat);

static int stat_filename(int dirfd, const char *filename, struct stat *s, unsigned int flags)
{
	int ret;
	struct dentry *dentry;
	struct inode *inode;
	struct path path = {};

	ret = filename_lookup(dirfd, getname(filename), flags, &path);
	if (ret)
		goto out;

	dentry = path.dentry;

	if (d_is_negative(dentry)) {
		ret = -ENOENT;
		goto out_put;
	}

	inode = d_inode(dentry);

	stat_inode(inode, s);

	ret = 0;
out_put:
	path_put(&path);
out:
	return errno_set(ret);
}

int statat(int dirfd, const char *filename, struct stat *s)
{
	return stat_filename(dirfd, filename, s, LOOKUP_FOLLOW);
}
EXPORT_SYMBOL(statat);

int lstatat(int dirfd, const char *filename, struct stat *s)
{
	return stat_filename(dirfd, filename, s, 0);
}
EXPORT_SYMBOL(lstatat);

static char *__dpath(struct dentry *dentry, struct dentry *root)
{
	char *res, *ppath;

	if (dentry == root)
		return NULL;
	if (dentry == d_root)
		return NULL;

	while (IS_ROOT(dentry)) {
		struct fs_device *fsdev;

		for_each_fs_device(fsdev) {
			if (dentry == fsdev->vfsmount.mnt_root) {
				dentry = fsdev->vfsmount.mountpoint;
				break;
			}
		}
	}

	ppath = __dpath(dentry->d_parent, root);
	if (ppath)
		res = basprintf("%s/%s", ppath, dentry->name);
	else
		res = basprintf("/%s", dentry->name);
	free(ppath);

	return res;
}

/**
 * dpath - return path of a dentry
 * @dentry: The dentry to return the path from
 * @root:   The dentry up to which the path is followed
 *
 * Get the path of a dentry. The path is followed up to
 * @root or the root ("/") dentry, whatever is found first.
 *
 * Return: Dynamically allocated string containing the path
 */
char *dpath(struct dentry *dentry, struct dentry *root)
{
	char *res;

	if (dentry == root)
		return strdup("/");

	res = __dpath(dentry, root);

	return res;
}

/**
 * canonicalize_path - resolve links in path
 *
 * @dirfd: directory file descriptor to look up relative to
 * @pathname: The input path
 *
 * This function resolves all links in @pathname and returns
 * a path without links in it.
 *
 * Return: Path with links resolved. Allocated, must be freed after use.
 */
char *canonicalize_path(int dirfd, const char *pathname)
{
	char *res = NULL;
	struct path path;
	int ret;

	ret = filename_lookup(dirfd, getname(pathname), LOOKUP_FOLLOW, &path);
	if (ret)
		goto out;

	res = dpath(path.dentry, d_root);
out:
	errno_set(ret);
	return res;
}

const char *getcwd(void)
{
	return cwd;
}
EXPORT_SYMBOL(getcwd);

int chdir(const char *pathname)
{
	char *realpath;
	struct path path;
	int ret;

	ret = filename_lookup(AT_FDCWD, getname(pathname), LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &path);
	if (ret)
		goto out;

	if (!d_is_dir(path.dentry)) {
		ret = -ENOTDIR;
		goto out;
	}

	realpath = dpath(path.dentry, d_root);
	strcpy(cwd, realpath);
	free(realpath);
	cwd_dentry = path.dentry;
	cwd_mnt = path.mnt;

	ret = 0;

out:
	return errno_set(ret);
}
EXPORT_SYMBOL(chdir);

char *pushd(const char *dir)
{
	char *oldcwd;
	int ret;

	oldcwd = strdup(getcwd());
	if (!oldcwd)
		return NULL;

	ret = chdir(dir);
	if (ret) {
		free(oldcwd);
		return NULL;
	}

	return oldcwd;
}

int popd(char *oldcwd)
{
	int ret;

	if (!oldcwd)
		return 0;

	ret = chdir(oldcwd);
	free(oldcwd);
	return ret;
}

static bool cdev_partname_equal(const struct cdev *a,
				const struct cdev *b)
{
	return a->partname && b->partname &&
		!strcmp(a->partname, b->partname);
}

static char *get_linux_mmcblkdev(const struct cdev *root_cdev)
{
	struct cdev *cdevm = root_cdev->master, *cdev;
	int id, partnum;

	if (!IS_ENABLED(CONFIG_MMCBLKDEV_ROOTARG))
		return NULL;
	if (!cdevm || !cdev_is_mci_main_part_dev(cdevm))
		return NULL;

	id = of_alias_get_id(cdev_of_node(cdevm), "mmc");
	if (id < 0)
		return NULL;

	partnum = 1; /* linux partitions are 1 based */
	list_for_each_entry(cdev, &cdevm->partitions, partition_entry) {

		/*
		 * Partname is not guaranteed but this partition cdev is listed
		 * in the partitions list so we need to count it instead of
		 * skipping it.
		 */
		if (cdev_partname_equal(root_cdev, cdev))
			return basprintf("root=/dev/mmcblk%dp%d", id, partnum);
		partnum++;
	}

	return NULL;
}

char *cdev_get_linux_rootarg(const struct cdev *cdev)
{
	char *str;

	if (!cdev)
		return NULL;

	str = get_linux_mmcblkdev(cdev);
	if (str)
		return str;

	if (cdev->partuuid[0] != 0)
		return basprintf("root=PARTUUID=%s", cdev->partuuid);

	return NULL;
}

/*
 * Mount a device to a directory.
 * We do this by registering a new device on which the filesystem
 * driver will match.
 */
int mount(const char *device, const char *fsname, const char *pathname,
		const char *fsoptions)
{
	struct fs_device *fsdev;
	int ret;
	struct path path = {};

	if (d_root) {
		ret = filename_lookup(AT_FDCWD, getname(pathname), LOOKUP_FOLLOW, &path);
		if (ret)
			goto out;

		if (!d_is_dir(path.dentry)) {
			ret = -ENOTDIR;
			goto out;
		}

		if (IS_ROOT(path.dentry) || d_mountpoint(path.dentry)) {
			ret = -EBUSY;
			goto out;
		}
	} else {
		if (pathname[0] != '/' || pathname[1])
			return -EINVAL;
	}

	if (!fsoptions)
		fsoptions = "";

	debug("mount: %s on %s type %s, options=%s\n",
			device, pathname, fsname, fsoptions);

	if (!fsname)
		fsname = fs_detect(device, fsoptions);

	if (!fsname) {
		ret = -ENOENT;
		goto out;
	}

	fsdev = xzalloc(sizeof(struct fs_device));
	fsdev->backingstore = xstrdup(device);
	dev_set_name(&fsdev->dev, fsname);
	fsdev->dev.id = get_free_deviceid(fsdev->dev.name);
	fsdev->dev.bus = &fs_bus;
	fsdev->options = xstrdup(fsoptions);

	init_super(&fsdev->sb);

	if (path.mnt)
		mntget(path.mnt);

	if (d_root)
		fsdev->path = dpath(path.dentry, d_root);

	ret = register_device(&fsdev->dev);
	if (ret)
		goto err_register;

	if (!fsdev->dev.driver) {
		/*
		 * Driver didn't accept the device or no driver for this
		 * device. Bail out
		 */
		ret = -EINVAL;
		goto err_no_driver;
	}

	if (d_root) {
		fsdev->vfsmount.mountpoint = path.dentry;
		fsdev->vfsmount.parent = path.mnt;
		fsdev->vfsmount.mountpoint->d_flags |= DCACHE_MOUNTED;
	} else {
		d_root = fsdev->sb.s_root;
		path.dentry = d_root;
		mnt_root = &fsdev->vfsmount;
		fsdev->vfsmount.mountpoint = d_root;
		fsdev->vfsmount.parent = &fsdev->vfsmount;
		fsdev->path = xstrdup("/");

		cwd_dentry = d_root;
		cwd_mnt = mnt_root;
	}

	fsdev->vfsmount.mnt_root = fsdev->sb.s_root;

	if (!fsdev->linux_rootarg) {
		char *str;

		str = cdev_get_linux_rootarg(fsdev->cdev);
		if (str)
			fsdev_set_linux_rootarg(fsdev, str);
	}

	path_put(&path);

	return 0;

err_no_driver:
	unregister_device(&fsdev->dev);
err_register:
	fs_remove(&fsdev->dev);
out:
	path_put(&path);

	return errno_set(ret);
}
EXPORT_SYMBOL(mount);

int umount(const char *pathname)
{
	struct fs_device *fsdev = NULL, *f;
	struct path path = {};
	int ret;

	ret = filename_lookup(AT_FDCWD, getname(pathname), LOOKUP_FOLLOW, &path);
	if (ret)
		return ret;

	if (path.dentry == d_root) {
		path_put(&path);
		return -EBUSY;
	}

	for_each_fs_device(f) {
		if (path.dentry == f->vfsmount.mnt_root) {
			fsdev = f;
			break;
		}
	}

	path_put(&path);

	if (!fsdev) {
		struct cdev *cdev = cdev_open_by_name(pathname, O_RDWR);

		if (cdev) {
			cdev_close(cdev);
			return umount_by_cdev(cdev);
		}
	}

	if (!fsdev)
		return errno_set(-EFAULT);

	return fsdev_umount(fsdev);
}
EXPORT_SYMBOL(umount);

#ifdef CONFIG_FS_AUTOMOUNT

#define AUTOMOUNT_IS_FILE (1 << 0)

struct automount {
	char *path;
	struct dentry *dentry;
	char *cmd;
	struct list_head list;
	unsigned int flags;
};

static LIST_HEAD(automount_list);

static void automount_remove_dentry(struct dentry *dentry)
{
	struct automount *am;

	list_for_each_entry(am, &automount_list, list) {
		if (dentry == am->dentry)
			goto found;
	}

	return;
found:
	list_del(&am->list);
	dput(am->dentry);
	free(am->path);
	free(am->cmd);
	free(am);
}

void automount_remove(const char *pathname)
{
	struct path path;
	int ret;

	ret = filename_lookup(AT_FDCWD, getname(pathname), LOOKUP_FOLLOW, &path);
	if (ret)
		return;

	automount_remove_dentry(path.dentry);

	path_put(&path);
}
EXPORT_SYMBOL(automount_remove);

int automount_add(const char *pathname, const char *cmd)
{
	struct automount *am = xzalloc(sizeof(*am));
	struct path path;
	int ret;

	ret = filename_lookup(AT_FDCWD, getname(pathname), LOOKUP_FOLLOW, &path);
	if (ret)
		return ret;

	if (!d_is_dir(path.dentry)) {
		ret = -ENOTDIR;
		goto out;
	}

	am->path = dpath(path.dentry, d_root);
	am->dentry = dget(path.dentry);
	am->dentry->d_flags |= DCACHE_NEED_AUTOMOUNT;
	am->cmd = xstrdup(cmd);

	automount_remove_dentry(am->dentry);

	list_add_tail(&am->list, &automount_list);

	ret = 0;
out:
	path_put(&path);

	return ret;
}
EXPORT_SYMBOL(automount_add);

void cdev_create_default_automount(struct cdev *cdev)
{
	char *path, *cmd;

	path = basprintf("/mnt/%s", cdev->name);
	cmd = basprintf("mount %s", cdev->name);

	make_directory(path);
	automount_add(path, cmd);

	free(cmd);
	free(path);
}

void automount_print(void)
{
	struct automount *am;

	list_for_each_entry(am, &automount_list, list)
		printf("%-20s %s\n", am->path, am->cmd);
}
EXPORT_SYMBOL(automount_print);

static int automount_mount(struct dentry *dentry)
{
	struct automount *am;
	int ret = -ENOENT;
	static int in_automount;

	if (in_automount)
		return -EINVAL;

	in_automount++;

	list_for_each_entry(am, &automount_list, list) {
		if (am->dentry != dentry)
			continue;

		setenv("automount_path", am->path);
		export("automount_path");
		ret = run_command(am->cmd);
		unsetenv("automount_path");

		if (ret) {
			printf("running automount command '%s' failed\n",
					am->cmd);
			ret = -ENODEV;
		} else if (!(dentry->d_flags & DCACHE_MOUNTED)) {
			printf("automount command '%s' didn't mount anything\n",
					am->cmd);
			ret = -ENODEV;
		}

		break;
	}

	in_automount--;

	return ret;
}

BAREBOX_MAGICVAR(automount_path, "mountpath passed to automount scripts");

#else
static int automount_mount(struct dentry *dentry)
{
	return 0;
}
#endif /* CONFIG_FS_AUTOMOUNT */

#ifdef DEBUG

/*
 * Some debug commands, helpful to debug the dcache implementation
 */

static int do_lookup_dentry(int argc, char *argv[])
{
	struct path path;
	int ret;
	char *canon;
	char mode[16];

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	ret = filename_lookup(AT_FDCWD, getname(argv[1]), 0, &path);
	if (ret) {
		printf("Cannot lookup path \"%s\": %s\n",
		       argv[1], strerror(-ret));
		return 1;
	}

	canon = canonicalize_path(AT_FDCWD, argv[1]);

	printf("path \"%s\":\n", argv[1]);
	printf("dentry: 0x%p\n", path.dentry);
	printf("dentry refcnt: %d\n", path.dentry->d_count);
	if (path.dentry->d_inode) {
		struct inode *inode = path.dentry->d_inode;
		printf("inode: 0x%p\n", inode);
		printf("inode refcnt: %d\n", inode->i_count);
		printf("Type: %s\n", mkmodestr(inode->i_mode, mode));
	}
	printf("canonical path: \"%s\"\n", canon);

	path_put(&path);
	free(canon);

        return 0;
}

BAREBOX_CMD_START(lookup_dentry)
        .cmd            = do_lookup_dentry,
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END

static struct dentry *debug_follow_mount(struct dentry *dentry)
{
	struct fs_device *fsdev;
	unsigned managed = dentry->d_flags;

	if (managed & DCACHE_MOUNTED) {
		for_each_fs_device(fsdev) {
			if (dentry == fsdev->vfsmount.mountpoint)
				return fsdev->vfsmount.mnt_root;
		}
		return NULL;
	} else {
		return dentry;
	}
}

static void debug_dump_dentries(struct dentry *parent, int indent)
{
	int i;
	struct dentry *dentry, *mp;

	for (i = 0; i < indent; i++)
		printf("\t");
again:
	printf("%s d: %p refcnt: %d, inode %p refcnt %d\n",
	       parent->name, parent, parent->d_count, parent->d_inode,
		parent->d_inode ? parent->d_inode->i_count : -1);

	mp = debug_follow_mount(parent);
	if (mp != parent) {
		for (i = 0; i < indent; i++)
			printf("\t");
		printf("MOUNT: ");

		parent = mp;

		goto again;
	}

	list_for_each_entry(dentry, &parent->d_subdirs, d_child)
		debug_dump_dentries(dentry, indent + 1);
}

static int do_debug_fs_dump(int argc, char *argv[])
{
	debug_dump_dentries(d_root, 0);

        return 0;
}

BAREBOX_CMD_START(debug_fs_dump)
        .cmd            = do_debug_fs_dump,
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END
#endif
