/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DIRENT_H
#define __DIRENT_H

#include <linux/list.h>
#include <linux/path.h>

struct dirent {
	char d_name[256];
};

typedef struct dir {
	struct device *dev;
	struct fs_driver *fsdrv;
	struct dirent d;
	void *priv; /* private data for the fs driver */
	struct path path;
	struct list_head entries;
} DIR;

DIR *opendir(const char *pathname);
struct dirent *readdir(DIR *dir);
int unreaddir(DIR *dir, const struct dirent *d);
int closedir(DIR *dir);

#endif /* __DIRENT_H */
