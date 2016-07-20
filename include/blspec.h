#ifndef __LOADER_H__
#define __LOADER_H__

#include <linux/list.h>
#include <menu.h>

struct bootentries {
	struct list_head entries;
	struct menu *menu;
};

struct bootentry {
	struct list_head list;
	struct menu_entry me;
	char *title;
	char *description;
	int (*boot)(struct bootentry *entry, int verbose, int dryrun);
	void (*release)(struct bootentry *entry);
};

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

int bootentries_add_entry(struct bootentries *entries, struct bootentry *entry);

#define bootentries_for_each_entry(bootentries, entry) \
	list_for_each_entry(entry, &bootentries->entries, list)

#endif /* __LOADER_H__ */
