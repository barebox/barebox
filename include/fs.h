#ifndef __FS_H
#define __FS_H

#include <driver.h>

#define FS_TYPE_CRAMFS 1
#define FS_TYPE_RAMFS  2

#define PATH_MAX       1024        /* include/linux/limits.h */

struct partition;
struct node_d;

struct dirent {
	unsigned long mode;
	unsigned long size;
	char name[256];
};

struct dir {
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	struct node_d *node;
	struct dirent d;
	void *priv; /* private data for the fs driver */
};

#define FS_DRIVER_NO_DEV	1

struct fs_driver_d {
	ulong type;
	char *name;
	int (*load) (char *dst, struct device_d *dev, const char *filename);
	int (*probe) (struct device_d *dev);
	int (*create)(struct device_d *dev, const char *pathname, ulong type);
	int (*mkdir)(struct device_d *dev, const char *pathname);
	struct handle_d *(*open)(struct device_d *dev, const char *pathname);

	struct dir* (*opendir)(struct device_d *dev, const char *pathname);
	struct dirent* (*readdir)(struct device_d *dev, struct dir *dir);
	int (*closedir)(struct device_d *dev, struct dir *dir);

	struct driver_d drv;

	unsigned long flags;
};

struct fs_device_d {
	struct device_d *parent; /* the device we are associated with */
	struct device_d dev; /* our own device */

	struct fs_driver_d *driver;
};

int register_filesystem(struct device_d *dev, char *fsname);
//int unregister_filesystem(struct device_d *dev);

int register_fs_driver(struct fs_driver_d *new_fs_drv);

char *mkmodestr(unsigned long mode, char *str);

#endif /* __FS_H */
