/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOT_H
#define __BOOT_H

#include <of.h>
#include <menu.h>
#include <environment.h>
#include <bootm-overrides.h>
#include <bootargs.h>

struct bootentries {
	struct list_head entries;
	struct menu *menu;
};

struct bootentry {
	struct list_head list;
	struct menu_entry me;
	const char *title;
	char *description;
	int (*boot)(struct bootentry *entry, int verbose, int dryrun);
	void (*release)(struct bootentry *entry);
	struct bootm_overrides overrides;
};

int bootentries_add_entry(struct bootentries *entries, struct bootentry *entry);

struct bootentry_provider {
	const char *name;
	int (*generate)(struct bootentries *bootentries, const char *name);
	/* internal fields */
	struct list_head list;
};

int bootentry_register_provider(struct bootentry_provider *provider);
struct bootentry_provider *get_bootentry_provider(const char *name);

#define bootentries_for_each_entry(bootentries, entry) \
	list_for_each_entry(entry, &bootentries->entries, list)

struct watchdog;

void boot_set_default(const char *boot_default);
void boot_set_watchdog_timeout(unsigned int timeout);
struct watchdog *boot_get_enabled_watchdog(void);
struct bootentries *bootentries_alloc(void);
void bootentries_free(struct bootentries *bootentries);
int bootentry_create_from_name(struct bootentries *bootentries,
				      const char *name);
void bootsources_menu(struct bootentries *bootentries, unsigned default_entry, int timeout);
void bootsources_list(struct bootentries *bootentries);
int boot_entry(struct bootentry *be, int verbose, int dryrun);

#endif /* __BOOT_H */
