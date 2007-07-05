#ifndef __FS_H
#define __FS_H

#include <driver.h>

#define FS_TYPE_CRAMFS 1
#define FS_TYPE_RAMFS  2

#define PATH_MAX       1024        /* include/linux/limits.h */

struct partition;
struct node_d;
struct stat;

struct dirent {
	char name[256];
};

struct dir {
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	struct node_d *node;
	struct dirent d;
	void *priv; /* private data for the fs driver */
};

typedef struct filep {
	struct device_d *dev;
	ulong pos;
	char used;
	int no;
	void *inode; /* private to the filesystem driver */
} FILE;

#define FS_DRIVER_NO_DEV	1

struct fs_driver_d {
	ulong type;
	char *name;
	int (*probe) (struct device_d *dev);
	int (*create)(struct device_d *dev, const char *pathname, ulong type);
	int (*mkdir)(struct device_d *dev, const char *pathname);

	int (*open)(struct device_d *dev, FILE *f, const char *pathname);
	int (*close)(struct device_d *dev, FILE *f);
	int (*read)(struct device_d *dev, FILE *f, void *buf, size_t size);
	int (*write)(struct device_d *dev, FILE *f, void *buf, size_t size);

	struct dir* (*opendir)(struct device_d *dev, const char *pathname);
	struct dirent* (*readdir)(struct device_d *dev, struct dir *dir);
	int (*closedir)(struct device_d *dev, struct dir *dir);
	int (*stat)(struct device_d *dev, const char *file, struct stat *stat);

	struct driver_d drv;

	unsigned long flags;
};

struct fs_device_d {
	struct device_d *parent; /* the device we are associated with */
	struct device_d dev; /* our own device */

	struct fs_driver_d *driver;
};

int open(const char *pathname, int flags);
int close(int fd);
int read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int ls(const char *path);
int mkdir (const char *pathname);
int mount (struct device_d *dev, char *fsname, char *path);
int umount(const char *pathname);

struct dir *opendir(const char *pathname);
struct dirent *readdir(struct dir *dir);
int closedir(struct dir *dir);

char *mkmodestr(unsigned long mode, char *str);

struct mtab_entry *get_mtab_entry_by_path(const char *path);

struct mtab_entry {
	char path[PATH_MAX];
	struct mtab_entry *next;
	struct device_d *dev;
};

void normalise_path(char *path);

#endif /* __FS_H */
