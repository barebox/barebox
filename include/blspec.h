#ifndef __LOADER_H__
#define __LOADER_H__

#include <linux/list.h>
#include <boot.h>

struct blspec_entry {
	struct bootentry entry;

	struct device_node *node;
	struct cdev *cdev;
	char *rootpath;
	char *configpath;
};

int blspec_entry_var_set(struct blspec_entry *entry, const char *name,
		const char *val);
const char *blspec_entry_var_get(struct blspec_entry *entry, const char *name);

int blspec_scan_devices(struct bootentries *bootentries);

int blspec_scan_device(struct bootentries *bootentries, struct device_d *dev);
int blspec_scan_devicename(struct bootentries *bootentries, const char *devname);
int blspec_scan_directory(struct bootentries *bootentries, const char *root);

#endif /* __LOADER_H__ */
