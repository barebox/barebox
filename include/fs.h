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
#include <linux/fs.h>

#define PATH_MAX       1024        /* include/linux/limits.h */

struct partition;
struct node_d;
struct stat;

typedef struct filep {
	struct fs_device_d *fsdev; /* The device this FILE belongs to              */
	char *path;
	loff_t pos;            /* current position in stream                   */
#define FILE_SIZE_STREAM	((loff_t) -1)
	loff_t size;           /* The size of this inode                       */
	ulong flags;          /* the O_* flags from open                      */

	void *priv;         /* private to the filesystem driver              */

	/* private fields. Mapping between FILE and filedescriptor number     */
	int no;
	char in_use;
} FILE;

#define FS_DRIVER_NO_DEV	1

struct fs_driver_d {
	int (*probe) (struct device_d *dev);
	int (*mkdir)(struct device_d *dev, const char *pathname);
	int (*rmdir)(struct device_d *dev, const char *pathname);

	/* create a file. The file is guaranteed to not exist */
	int (*create)(struct device_d *dev, const char *pathname, mode_t mode);
	int (*unlink)(struct device_d *dev, const char *pathname);

	/* Truncate a file to given size */
	int (*truncate)(struct device_d *dev, FILE *f, ulong size);

	int (*symlink)(struct device_d *dev, const char *pathname,
		       const char *newpath);
	int (*readlink)(struct device_d *dev, const char *pathname, char *name,
			size_t size);

	int (*open)(struct device_d *dev, FILE *f, const char *pathname);
	int (*close)(struct device_d *dev, FILE *f);
	int (*read)(struct device_d *dev, FILE *f, void *buf, size_t size);
	int (*write)(struct device_d *dev, FILE *f, const void *buf, size_t size);
	int (*flush)(struct device_d *dev, FILE *f);
	loff_t (*lseek)(struct device_d *dev, FILE *f, loff_t pos);

	struct dir* (*opendir)(struct device_d *dev, const char *pathname);
	struct dirent* (*readdir)(struct device_d *dev, struct dir *dir);
	int (*closedir)(struct device_d *dev, DIR *dir);
	int (*stat)(struct device_d *dev, const char *file, struct stat *stat);

	int (*ioctl)(struct device_d *dev, FILE *f, int request, void *buf);
	int (*erase)(struct device_d *dev, FILE *f, loff_t count,
			loff_t offset);
	int (*protect)(struct device_d *dev, FILE *f, size_t count,
			loff_t offset, int prot);

	int (*memmap)(struct device_d *dev, FILE *f, void **map, int flags);

	struct driver_d drv;

	enum filetype type;

	unsigned long flags;
};

#define dev_to_fs_device(d) container_of(d, struct fs_device_d, dev)

extern struct list_head fs_device_list;
#define for_each_fs_device(f) list_for_each_entry(f, &fs_device_list, list)
#define for_each_fs_device_safe(tmp, f) list_for_each_entry_safe(f, tmp, &fs_device_list, list)
extern struct bus_type fs_bus;

struct fs_device_d {
	char *backingstore; /* the device we are associated with */
	struct device_d dev; /* our own device */

	struct fs_driver_d *driver;

	struct cdev *cdev;
	bool loop;
	char *path;
	struct device_d *parent_device;
	struct list_head list;
	char *options;
	char *linux_rootarg;
};

/*
 * Some filesystems i.e. tftpfs only support lseek into one direction.
 * To detect this limited functionality we add this extra function.
 * Additionaly we also return 0 if we even can not seek forward.
 */
static inline int can_lseek_backward(int fd)
{
	int ret;

	ret = lseek(fd, 1, SEEK_SET);
	if (ret < 0)
		return 0;

	ret = lseek(fd, 0, SEEK_SET);
	if (ret < 0)
		return 0;

	return ret;
}

#define drv_to_fs_driver(d) container_of(d, struct fs_driver_d, drv)

int flush(int fd);
int umount_by_cdev(struct cdev *cdev);

/* not-so-standard functions */
#define ERASE_SIZE_ALL	((loff_t) - 1)
int erase(int fd, loff_t count, loff_t offset);
int protect(int fd, size_t count, loff_t offset, int prot);
int protect_file(const char *file, int prot);
void *memmap(int fd, int flags);

#define FILESIZE_MAX	((loff_t)-1)

#define PROT_READ	1
#define PROT_WRITE	2

#define LS_RECURSIVE	1
#define LS_SHOWARG	2
#define LS_COLUMN	4
int ls(const char *path, ulong flags);

char *mkmodestr(unsigned long mode, char *str);

/*
 * This function turns 'path' into an absolute path and removes all occurrences
 * of "..", "." and double slashes. The returned string must be freed wit free().
 */
char *normalise_path(const char *path);

char *canonicalize_path(const char *pathname);

char *get_mounted_path(const char *path);

struct cdev *get_cdev_by_mountpath(const char *path);

/* Register a new filesystem driver */
int register_fs_driver(struct fs_driver_d *fsdrv);

void automount_remove(const char *_path);
int automount_add(const char *path, const char *cmd);
void automount_print(void);

int fsdev_open_cdev(struct fs_device_d *fsdev);
const char *cdev_get_mount_path(struct cdev *cdev);
const char *cdev_mount_default(struct cdev *cdev, const char *fsoptions);
void mount_all(void);

void fsdev_set_linux_rootarg(struct fs_device_d *fsdev, const char *str);
char *path_get_linux_rootarg(const char *path);

#endif /* __FS_H */
