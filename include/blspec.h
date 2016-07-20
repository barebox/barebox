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
};

struct blspec_entry {
	struct bootentry entry;

	struct device_node *node;
	struct cdev *cdev;
	char *rootpath;
	char *configpath;

	char *scriptpath;
};

int blspec_entry_var_set(struct blspec_entry *entry, const char *name,
		const char *val);
const char *blspec_entry_var_get(struct blspec_entry *entry, const char *name);

int blspec_boot(struct bootentry *entry, int verbose, int dryrun);

int blspec_scan_devices(struct bootentries *bootentries);

int blspec_scan_device(struct bootentries *bootentries, struct device_d *dev);
int blspec_scan_devicename(struct bootentries *bootentries, const char *devname);
int blspec_scan_directory(struct bootentries *bootentries, const char *root);

#define bootentries_for_each_entry(bootentries, entry) \
	list_for_each_entry(entry, &bootentries->entries, list)

static inline struct blspec_entry *blspec_entry_alloc(struct bootentries *bootentries)
{
	struct blspec_entry *entry;

	entry = xzalloc(sizeof(*entry));

	entry->node = of_new_node(NULL, NULL);

	list_add_tail(&entry->entry.list, &bootentries->entries);

	return entry;
}

static inline void blspec_entry_free(struct blspec_entry *entry)
{
	list_del(&entry->entry.list);
	of_delete_node(entry->node);
	free(entry->entry.me.display);
	free(entry->scriptpath);
	free(entry->configpath);
	free(entry->rootpath);
	free(entry);
}

static inline struct bootentries *blspec_alloc(void)
{
	struct bootentries *bootentries;

	bootentries = xzalloc(sizeof(*bootentries));
	INIT_LIST_HEAD(&bootentries->entries);

	if (IS_ENABLED(CONFIG_MENU))
		bootentries->menu = menu_alloc();

	return bootentries;
}

static inline void blspec_free(struct bootentries *bootentries)
{
	struct bootentry *be, *tmp;
	struct blspec_entry *entry;

	list_for_each_entry_safe(be, tmp, &bootentries->entries, list) {
		entry = container_of(be, struct blspec_entry, entry);
		blspec_entry_free(entry);
	}
	if (bootentries->menu)
		free(bootentries->menu->display);
	free(bootentries->menu);
	free(bootentries);
}

#endif /* __LOADER_H__ */
