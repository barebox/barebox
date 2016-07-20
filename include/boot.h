#ifndef __BOOT_H
#define __BOOT_H

#include <of.h>
#include <menu.h>
#include <environment.h>

#ifdef CONFIG_FLEXIBLE_BOOTARGS
const char *linux_bootargs_get(void);
int linux_bootargs_overwrite(const char *bootargs);
#else
static inline const char *linux_bootargs_get(void)
{
	return getenv("bootargs");
}

static inline int linux_bootargs_overwrite(const char *bootargs)
{
	return setenv("bootargs", bootargs);
}
#endif

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

int bootentries_add_entry(struct bootentries *entries, struct bootentry *entry);

#define bootentries_for_each_entry(bootentries, entry) \
	list_for_each_entry(entry, &bootentries->entries, list)

#endif /* __BOOT_H */
