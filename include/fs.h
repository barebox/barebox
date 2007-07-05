#ifndef __FS_H
#define __FS_H

#include <driver.h>

#define FS_TYPE_CRAMFS 1
#define FS_TYPE_RAMFS  2

struct partition;

struct fs_driver_d {
	ulong type;
	char *name;
	int (*ls) (struct device_d *dev, const char *filename);
	int (*load) (char *dst, struct device_d *dev, const char *filename);
	int (*probe) (struct device_d *dev);
	int (*create)(struct device_d *dev, const char *pathname, ulong type);
	struct handle_d *(*open)(struct device_d *dev, const char *pathname);

	struct driver_d drv;
};

struct fs_device_d {
	ulong type;
	char *name;
	struct device_d *parent; /* the device we are associated with */
	struct device_d dev; /* our own device */

	struct fs_driver_d *driver;
};

int register_filesystem(struct device_d *dev, char *fsname);
//int unregister_filesystem(struct device_d *dev);

int register_fs_driver(struct fs_driver_d *new_fs_drv);

#endif /* __FS_H */
