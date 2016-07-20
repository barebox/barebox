#ifndef __LOADER_H__
#define __LOADER_H__

#include <linux/list.h>
#include <menu.h>

struct bootentries {
	struct list_head entries;
	struct menu *menu;
};

struct blspec_entry {
	struct list_head list;
	struct device_node *node;
	struct cdev *cdev;
	char *rootpath;
	char *configpath;

	struct menu_entry me;

	char *scriptpath;
};

int blspec_entry_var_set(struct blspec_entry *entry, const char *name,
		const char *val);
const char *blspec_entry_var_get(struct blspec_entry *entry, const char *name);

int blspec_boot(struct blspec_entry *entry, int verbose, int dryrun);

int blspec_scan_devices(struct bootentries *bootentries);

int blspec_scan_device(struct bootentries *bootentries, struct device_d *dev);
int blspec_scan_devicename(struct bootentries *bootentries, const char *devname);
int blspec_scan_directory(struct bootentries *bootentries, const char *root);

#define blspec_for_each_entry(blspec, entry) \
	list_for_each_entry(entry, &blspec->entries, list)

static inline struct blspec_entry *blspec_entry_alloc(struct bootentries *bootentries)
{
	struct blspec_entry *entry;

	entry = xzalloc(sizeof(*entry));

	entry->node = of_new_node(NULL, NULL);

	list_add_tail(&entry->list, &bootentries->entries);

	return entry;
}

static inline void blspec_entry_free(struct blspec_entry *entry)
{
	list_del(&entry->list);
	of_delete_node(entry->node);
	free(entry->me.display);
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
	struct blspec_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &bootentries->entries, list)
		blspec_entry_free(entry);
	if (bootentries->menu)
		free(bootentries->menu->display);
	free(bootentries->menu);
	free(bootentries);
}

#endif /* __LOADER_H__ */
