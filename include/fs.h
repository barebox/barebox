/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __FS_H
#define __FS_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <driver.h>
#include <filetype.h>
#include <linux/bits.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/limits.h>

struct partition;
struct node_d;
struct stat;

/**
 * enum erase_type - Type of erase operation
 * @ERASE_TO_WRITE: Conduct low-level erase operation to prepare for a write
 *                  to all or part of the erased regions. This is required
 *                  for raw flashes, but can be omitted by flashes behind
 *                  a FTL that autoerases on write (e.g. eMMCs)
 * @ERASE_TO_CLEAR: Force an erase of the region. When read afterwards,
 *		    A fixed pattern should result, usually either all-zeroes
 *		    or all-ones depending on storage technology.
 *
 * Many managed flashes provide an erase operation, which need not be used
 * for regular writes, but is provided nonetheless for efficient clearing
 * of large regions of storage to a fixed bit pattern. The erase type allows
 * users to specify _why_ the erase operation is done, so the driver or
 * the core code can choose the most optimal operation.
 */
enum erase_type {
	ERASE_TO_WRITE,
	ERASE_TO_CLEAR
};

struct fs_driver {
	int (*probe) (struct device *dev);

	const struct fs_legacy_ops {
		int (*open)(struct device *dev, struct file *f, const char *pathname);
		int (*close)(struct device *dev, struct file *f);
		/* create a file. The file is guaranteed to not exist */
		int (*create)(struct device *dev, const char *pathname, mode_t mode);
		int (*unlink)(struct device *dev, const char *pathname);

		int (*mkdir)(struct device *dev, const char *pathname);
		int (*rmdir)(struct device *dev, const char *pathname);
		int (*symlink)(struct device *dev, const char *pathname,
			       const char *newpath);
		int (*readlink)(struct device *dev, const char *pathname, char *name,
				size_t size);
		struct dir* (*opendir)(struct device *dev, const char *pathname);
		struct dirent* (*readdir)(struct device *dev, struct dir *dir);
		int (*closedir)(struct device *dev, DIR *dir);
		int (*stat)(struct device *dev, const char *file, struct stat *stat);
		int (*read)(struct file *f, void *buf, size_t size);
		int (*write)(struct file *f, const void *buf, size_t size);
		int (*lseek)(struct file *f, loff_t pos);
		int (*ioctl)(struct file *f, unsigned int request, void *buf);
		int (*truncate)(struct file *f, loff_t size);
	} *legacy_ops;

	struct driver drv;

	enum filetype type;
};

#define dev_to_fs_device(d) container_of(d, struct fs_device, dev)

extern struct list_head fs_device_list;
#define for_each_fs_device(f) list_for_each_entry(f, &fs_device_list, list)
#define for_each_fs_device_safe(tmp, f) list_for_each_entry_safe(f, tmp, &fs_device_list, list)
extern struct bus_type fs_bus;

struct fs_device {
	char *backingstore; /* the device we are associated with */
	struct device dev; /* our own device */

	struct fs_driver *driver;

	struct cdev *cdev;
	bool loop;
	char *path;
	struct list_head list;
	char *options;
	char *linux_rootarg;

	struct super_block sb;

	struct vfsmount vfsmount;
};

bool __is_tftp_fs(const char *path);

static inline bool is_tftp_fs(const char *path)
{
	if (!IS_ENABLED(CONFIG_FS_TFTP))
		return false;

	return __is_tftp_fs(path);
}

#define drv_to_fs_driver(d) container_of(d, struct fs_driver, drv)

int flush(int fd);
int umount_by_cdev(struct cdev *cdev);

/* not-so-standard functions */
#define ERASE_SIZE_ALL	((loff_t) - 1)
int erase(int fd, loff_t count, loff_t offset, enum erase_type type);
int protect(int fd, size_t count, loff_t offset, int prot);
int discard_range(int fd, loff_t count, loff_t offset);
int protect_file(const char *file, int prot);
void *memmap(int fd, int flags);

#define MAP_FAILED ((void *)-1)

#define FILESIZE_MAX	((loff_t)-1)

#define PROT_READ	1
#define PROT_WRITE	2

#define LS_RECURSIVE	1
#define LS_SHOWARG	2
#define LS_COLUMN	4
int ls(const char *path, ulong flags);

char *mkmodestr(unsigned long mode, char *str);

void stat_print(int dirfd, const char *filename, const struct stat *st);
void cdev_print(const struct cdev *cdev);

char *canonicalize_path(int dirfd, const char *pathname);

struct fs_device *get_fsdevice_by_path(int dirfd, const char *path);

const char *get_mounted_path(const char *path);

struct cdev *get_cdev_by_mountpath(const char *path);

/* Register a new filesystem driver */
int register_fs_driver(struct fs_driver *fsdrv);

void automount_remove(const char *_path);
int automount_add(const char *path, const char *cmd);
void automount_print(void);

int fs_init_legacy(struct fs_device *fsdev);
int fsdev_open_cdev(struct fs_device *fsdev);
const char *cdev_get_mount_path(struct cdev *cdev);
const char *cdev_mount_default(struct cdev *cdev, const char *fsoptions);
const char *cdev_mount(struct cdev *cdev);
void mount_all(void);

void fsdev_set_linux_rootarg(struct fs_device *fsdev, const char *str);
char *path_get_linux_rootarg(const char *path);

static inline const char *devpath_to_name(const char *devpath)
{
	if (devpath && !strncmp(devpath, "/dev/", 5))
		return devpath + 5;

	return devpath;
}

const char *fs_detect(const char *filename, const char *fsoptions);

static inline char *filepath(struct file *f)
{
	return dpath(f->f_path.dentry, f->fsdev->vfsmount.mnt_root);
}

#endif /* __FS_H */
