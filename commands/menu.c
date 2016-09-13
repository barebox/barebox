/*
 * (C) Copyright 2009-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <readkey.h>
#include <menu.h>
#include <getopt.h>
#include <errno.h>
#include <libbb.h>
#include <linux/err.h>

typedef enum {
#if defined(CONFIG_CMD_MENU_MANAGEMENT)
	action_add,
	action_remove,
	action_select,
#endif
	action_list,
	action_show,
} menu_action;

struct cmd_menu {
	char		*menu;
	menu_action	action;
	char		*description;
	int		auto_select;
#if defined(CONFIG_CMD_MENU_MANAGEMENT)
	int		entry;
	int		re_entrant;
	char		*command;
	char		*submenu;
	int		num;
	menu_entry_type	type;
	int		box_state;
#endif
};

#if defined(CONFIG_CMD_MENU_MANAGEMENT)
#define OPTS		"m:earlc:d:RsSn:u:A:b:B:"
#define	is_entry(x)	((x)->entry)
#else
#define OPTS		"m:lsA:d:"
#define	is_entry(x)	(0)
#endif

#if defined(CONFIG_CMD_MENU_MANAGEMENT)
/*
 * menu -e -a -m <menu> -c <command> [-R] [-b 0|1 ] -d <description>
 * menu -e -a -m <menu> -u submenu -d [-b 0|1] <description>
 */
static int do_menu_entry_add(struct cmd_menu *cm)
{
	struct menu_entry *me;
	struct menu *m;

	if (!cm->menu || (!cm->command && !cm->submenu) || !cm->description)
		return -EINVAL;

	m = menu_get_by_name(cm->menu);

	if (!m) {
		eprintf("Menu '%s' not found\n", cm->menu);
		return -EINVAL;
	}

	if (cm->submenu)
		me = menu_add_submenu(m, cm->submenu, cm->description);
	else
		me = menu_add_command_entry(m, cm->description, cm->command,
					    cm->type);
	if (IS_ERR(me))
		return PTR_ERR(me);

	me->box_state = cm->box_state > 0 ? 1 : 0;

	if (!cm->submenu)
		me->non_re_ent = !cm->re_entrant;

	return 0;
}

/*
 * menu -e -r -m <name> -n <num>
 */
static int do_menu_entry_remove(struct cmd_menu *cm)
{
	struct menu *m;
	struct menu_entry *me;

	if (!cm->menu || cm->num < 0)
		return -EINVAL;

	m = menu_get_by_name(cm->menu);

	if (!m) {
		eprintf("Menu '%s' not found\n", cm->menu);
		return -EINVAL;
	}

	me = menu_entry_get_by_num(m, cm->num);

	if (!me) {
		eprintf("Entry '%i' not found\n", cm->num);
		return -EINVAL;
	}

	menu_remove_entry(m, me);

	menu_entry_free(me);

	return 0;
}

/*
 * menu -a -m <name> -d <description>
 */
static int do_menu_add(struct cmd_menu *cm)
{
	struct menu *m;
	int ret = -ENOMEM;

	if (!cm->menu || !cm->description)
		return -EINVAL;

	m = menu_alloc();

	if (!m)
		goto free;

	m->name = strdup(cm->menu);
	if (!m->name)
		goto free;

	menu_add_title(m, strdup(cm->description));

	ret = menu_add(m);

	if (ret)
		goto free;

	return 0;

free:
	eprintf("Menu '%s' add fail", cm->menu);
	if (ret == -EEXIST)
		eprintf(" already exist");
	eprintf("\n");

	menu_free(m);

	return ret;
}
/*
 * menu -r -m <name>
 */
static int do_menu_remove(struct cmd_menu *cm)
{
	struct menu *m;

	m = menu_get_by_name(cm->menu);

	if (!m) {
		eprintf("Menu '%s' not found\n", cm->menu);
		return -EINVAL;
	}

	menu_remove(m);

	menu_free(m);

	return 0;
}

/*
 * menu -m <menu> -S -n <entry num starting at 1>
 */
static int do_menu_select(struct cmd_menu *cm)
{
	struct menu *m;

	if (cm->num < 0)
		return -EINVAL;

	m = menu_get_by_name(cm->menu);

	if (!m) {
		eprintf("Menu '%s' not found\n", cm->menu);
		return -EINVAL;
	}

	if (menu_set_selected(m, cm->num) < 0) {
		eprintf("Entry '%d' not found\n", cm->num);
		return -EINVAL;
	}

	return 0;
}
#endif

/*
 * menu -s -m <menu> [-A <auto select delay>] [-d <display]
 */
static int do_menu_show(struct cmd_menu *cm)
{
	struct menu *m;

	if (cm->menu)
		m = menu_get_by_name(cm->menu);
	else
		m = menu_get_by_name("boot");

	if (!m)
		return -EINVAL;

	if (cm->auto_select != -EINVAL) {
		menu_set_auto_select(m, cm->auto_select);

		free(m->auto_display);

		m->auto_display = strdup(cm->description);
	}

	return menu_show(m);
}

static void print_entries(struct menu *m)
{
	struct menu_entry *me;

	list_for_each_entry(me, &m->entries, list)
		printf("%d: %s\n", me->num, me->display);
}

/*
 * menu -l
 * menu -e -l [menu]
 */
static int do_menu_list(struct cmd_menu *cm)
{
	struct menu* m = NULL;
	struct menu* menus = menu_get_menus();

	if (!menus)
		return 0;

	if (is_entry(cm)) {
		if (cm->menu)
			m = menu_get_by_name(cm->menu);

		if (m) {
			print_entries(m);
			return 0;
		}
	}

	list_for_each_entry(m, &menus->list, list) {
		printf("%s: ", m->name);
		if (m->display_lines) {
			static char outstr[256];
			int i;

			printf("\n");
			for (i = 0; i < m->display_lines; i++)
				/* Conform to menu rendering logic */
				if (IS_ENABLED(CONFIG_SHELL_HUSH)) {
					process_escape_sequence(m->display[i], outstr, 256);
					printf("\t%s\n", outstr);
				} else {
					printf("\t%s\n", m->display[i]);
				}
		} else {
			printf("%s\n", m->name);
		}
		if (is_entry(cm))
			print_entries(m);
	}

	return 0;
}

#if defined(CONFIG_CMD_MENU_MANAGEMENT)
static int do_menu_entry(struct cmd_menu *cm)
{
	switch(cm->action) {
	case action_list:
		return do_menu_list(cm);
	case action_remove:
		return do_menu_entry_remove(cm);
	case action_add:
		return do_menu_entry_add(cm);
	case action_select:
	case action_show:
		break;
	}
	return -EINVAL;
}
#else
static int do_menu_entry(struct cmd_menu *cm)
{
	return -EINVAL;
}
#endif

static int do_menu(int argc, char *argv[])
{
	struct cmd_menu cm;
	int opt;
	int ret = -EINVAL;

	if (!argc)
		return COMMAND_ERROR_USAGE;

	memset(&cm, 0, sizeof(struct cmd_menu));
#if defined(CONFIG_CMD_MENU_MANAGEMENT)
	cm.num = -EINVAL;
	cm.auto_select = -EINVAL;
#endif

	cm.action = action_show;

	while((opt = getopt(argc, argv, OPTS)) > 0) {
		switch(opt) {
		case 'm':
			cm.menu = optarg;
			break;
		case 'l':
			cm.action = action_list;
			break;
		case 's':
			cm.action = action_show;
			break;
		case 'A':
			cm.auto_select = simple_strtoul(optarg, NULL, 10);
			break;
		case 'd':
			cm.description = optarg;
			break;
#if defined(CONFIG_CMD_MENU_MANAGEMENT)
		case 'e':
			cm.entry = 1;
			break;
		case 'a':
			cm.action = action_add;
			break;
		case 'r':
			cm.action = action_remove;
			break;
		case 'c':
			cm.command = optarg;
			break;
		case 'u':
			cm.submenu = optarg;
			break;
		case 'R':
			cm.re_entrant = 1;
			break;
		case 'S':
			cm.action = action_select;
			break;
		case 'n':
			cm.num = simple_strtoul(optarg, NULL, 10);
			break;
		case 'b':
			cm.type = MENU_ENTRY_BOX;
			cm.box_state = simple_strtoul(optarg, NULL, 10);
			break;
		case 'B':
			cm.command = optarg;
			break;
#endif
		default:
			return 1;
		}
	}

	if (is_entry(&cm)) {
		ret = do_menu_entry(&cm);
		goto end;
	}

	switch(cm.action) {
	case action_list:
		ret = do_menu_list(&cm);
		break;
#if defined(CONFIG_CMD_MENU_MANAGEMENT)
	case action_remove:
		ret = do_menu_remove(&cm);
		break;
	case action_add:
		ret = do_menu_add(&cm);
		break;
	case action_select:
		ret = do_menu_select(&cm);
		break;
#endif
	case action_show:
		ret = do_menu_show(&cm);
		break;
	}

end:
	if (ret)
		return 0;

	return 1;
}

static const __maybe_unused char cmd_menu_help[] =
"Manage Menu:\n"
"  -m  menu\n"
"  -l  list\n"
"  -s  show\n"
#if defined(CONFIG_CMD_MENU_MANAGEMENT)
"Advanced menu management:\n"
"  -e  menu entry\n"
"  -a  add\n"
"  -r  remove\n"
"  -S  select\n"
#endif
"\n"
"Show menu:\n"
"  (-A auto select delay)\n"
"  (-d auto select description)\n"
"  menu -s -m MENU [-A delay] [-d auto_display]\n"
"\n"
"List menu:\n"
"  menu -l\n"
"\n"
#if defined(CONFIG_CMD_MENU_MANAGEMENT)
"Add a menu:\n"
"  menu -a -m NAME -d DESC\n"
"\n"
"Remove a menu:\n"
"  menu -r -m NAME\n"
"\n"
"Add an entry:\n"
"  (-R for do no exit the menu after executing the command)\n"
"  (-b for box style 1 for selected)\n"
"  (and optional -c for the command to run when we change the state)\n"
"  menu -e -a -m MENU -c COMMAND [-R] [-b 0|1] -d DESC\n"

"Add a submenu entry:\n"
"  (-R is not needed)\n"
"  (-b for box style 1 for selected)\n"
"  (and -c is not needed)\n"
"  menu -e -a -m MENU -u submenu -d [-b 0|1] DESC\n"
"\n"
"Remove an entry:\n"
"  menu -e -r -m NAME -n ENTRY\n"
"\n"
"Select an entry:\n"
"  menu -m <menu> -S -n ENTRY\n"
"\n"
"List menu:\n"
"  menu -e -l [menu]\n"
"\n"
"Menu examples:\n"
"  menu -a -m boot -d \"Boot Menu\"\n"
"  menu -e -a -m boot -c boot -d \"Boot\"\n"
"  menu -e -a -m boot -c reset -d \"Reset\"\n"
#else
"Menu example:\n"
#endif
"  menu -s -m boot\n"
;

BAREBOX_CMD_START(menu)
	.cmd		= do_menu,
	BAREBOX_CMD_DESC("create and display menus")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_menu_help)
BAREBOX_CMD_END
