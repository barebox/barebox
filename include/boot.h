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

int bootentry_register_provider(int (*fn)(struct bootentries *bootentries, const char *name));

#define bootentries_for_each_entry(bootentries, entry) \
	list_for_each_entry(entry, &bootentries->entries, list)

void boot_set_watchdog_timeout(unsigned int timeout);
struct bootentries *bootentries_alloc(void);
void bootentries_free(struct bootentries *bootentries);
int bootentry_create_from_name(struct bootentries *bootentries,
				      const char *name);
void bootsources_menu(struct bootentries *bootentries, int timeout);
void bootsources_list(struct bootentries *bootentries);
int boot_entry(struct bootentry *be, int verbose, int dryrun);

#endif /* __BOOT_H */
