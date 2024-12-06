/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * (C) Copyright 2009-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 */

#ifndef __MENU_H__
#define __MENU_H__

#include <linux/list.h>
#include <malloc.h>

struct menu;

typedef enum {
	MENU_ENTRY_NORMAL = 0,
	MENU_ENTRY_BOX,
} menu_entry_type;

struct menu_entry {
	int num;
	const char *display;
	void (*action)(struct menu *m, struct menu_entry *me);
	void (*free)(struct menu_entry *me);
	int non_re_ent;

	/* MENU_ENTRY_BOX */
	int box_state;
	void (*box_action)(struct menu *m, struct menu_entry *me);

	menu_entry_type type;

	struct list_head list;
};

struct menu {
	const char *name;
	/* Multiline title */
	const char **display;
	/* Number of lines */
	int display_lines;

	int auto_select;
	const char *auto_display;

	struct list_head list;
	struct list_head entries;

	int nb_entries;

	struct menu_entry *selected;
	void *priv;
};

/*
 * menu functions
 */
static inline struct menu* menu_alloc(void)
{
	struct menu *m;

	m = calloc(1, sizeof(struct menu));
	if (m) {
		INIT_LIST_HEAD(&m->entries);
		m->nb_entries = 0;
		m->auto_select = -1;
	}
	return m;
}
struct menu_entry *menu_add_submenu(struct menu *parent,
				    const char *submenu,
				    const char *display);
struct menu_entry *menu_add_command_entry(struct menu *m,
					  const char *display,
					  const char *command,
					  menu_entry_type type);
void menu_free(struct menu *m);
int menu_add(struct menu* m);
void menu_remove(struct menu *m);
struct menu* menu_get_by_name(const char *name);
int menu_show(struct menu *m);
int menu_set_selected_entry(struct menu *m, struct menu_entry* me);
int menu_set_selected(struct menu *m, int num);
int menu_set_auto_select(struct menu *m, int delay);
struct menu* menu_get_menus(void);
void menu_add_title(struct menu *m, const char *display);

/*
 * menu entry functions
 */
static inline struct menu_entry* menu_entry_alloc(void)
{
	return calloc(1, sizeof(struct menu_entry));
}
void menu_entry_free(struct menu_entry *me);
int menu_add_entry(struct menu *m, struct menu_entry* me);
void menu_remove_entry(struct menu *m, struct menu_entry *me);
struct menu_entry* menu_entry_get_by_num(struct menu* m, int num);

/*
 * menu entry action functions
 */
void menu_action_exit(struct menu *m, struct menu_entry *me);

int menutree(const char *path, int toplevel);

#endif /* __MENU_H__ */
