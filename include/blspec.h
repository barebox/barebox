#ifndef __LOADER_H__
#define __LOADER_H__

#include <linux/list.h>
#include <menu.h>

struct blspec {
	struct list_head entries;
	struct menu *menu;
};

struct blspec_entry {
	struct list_head list;
	struct device_node *node;
	struct cdev *cdev;
	char *rootpath;
	char *configpath;
	bool boot_default;
	bool boot_once;

	struct menu_entry me;

	char *scriptpath;
};

int blspec_entry_var_set(struct blspec_entry *entry, const char *name,
		const char *val);
const char *blspec_entry_var_get(struct blspec_entry *entry, const char *name);

int blspec_entry_save(struct blspec_entry *entry, const char *path);

int blspec_boot(struct blspec_entry *entry, int verbose, int dryrun);

int blspec_boot_hwdevice(const char *devname, int verbose, int dryrun);

void blspec_scan_devices(struct blspec *blspec);

struct blspec_entry *blspec_entry_default(struct blspec *l);
int blspec_scan_hwdevice(struct blspec *blspec, const char *devname);

#define blspec_for_each_entry(blspec, entry) \
	list_for_each_entry(entry, &blspec->entries, list)

static inline struct blspec_entry *blspec_entry_alloc(struct blspec *blspec)
{
	struct blspec_entry *entry;

	entry = xzalloc(sizeof(*entry));

	entry->node = of_new_node(NULL, NULL);

	list_add_tail(&entry->list, &blspec->entries);

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

static inline struct blspec *blspec_alloc(void)
{
	struct blspec *blspec;

	blspec = xzalloc(sizeof(*blspec));
	INIT_LIST_HEAD(&blspec->entries);

	if (IS_ENABLED(CONFIG_MENU))
		blspec->menu = menu_alloc();

	return blspec;
}

static inline void blspec_free(struct blspec *blspec)
{
	struct blspec_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &blspec->entries, list)
		blspec_entry_free(entry);
	if (blspec->menu)
		free(blspec->menu->display);
	free(blspec->menu);
	free(blspec);
}

#endif /* __LOADER_H__ */
