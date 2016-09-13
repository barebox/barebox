/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <environment.h>
#include <libbb.h>
#include <common.h>
#include <command.h>
#include <glob.h>
#include <menu.h>
#include <fs.h>
#include <shell.h>
#include <libfile.h>

#include <linux/ctype.h>
#include <linux/stat.h>

struct menutree {
	char *action;
	struct menu_entry me;
};

static void menutree_action_subdir(struct menu *m, struct menu_entry *me)
{
	struct menutree *mt = container_of(me, struct menutree, me);

	menutree(mt->action, 0);
}

static void menutree_action(struct menu *m, struct menu_entry *me)
{
	struct menutree *mt = container_of(me, struct menutree, me);

	run_command(mt->action);
}

static void setenv_bool(const char *var, bool val)
{
	const char *str;

	if (val)
		str = "1";
	else
		str = "0";

	setenv(var, str);
}

static void menutree_box(struct menu *m, struct menu_entry *me)
{
	struct menutree *mt = container_of(me, struct menutree, me);

	setenv_bool(mt->action, me->box_state);
}

static void menutree_entry_free(struct menu_entry *me)
{
	struct menutree *mt = container_of(me, struct menutree, me);

	free(mt->action);
	free(mt->me.display);
	free(mt);
}

/*
 * menutree - show a menu constructed from a directory structure
 * @path: the path to the directory structure
 *
 * Each menu entry is described by a subdirectory. Each subdirectory
 * can contain the following files which further describe the entry:
 *
 * title - A file containing the title of the entry as shown in the menu
 * box - If present, the entry is a 'bool' entry. The file contains a variable
 *       name from which the current state of the bool is taken from and saved
 *       to.
 * action - if present this file contains a shell script which is executed when
 *          when the entry is selected.
 *
 * If neither 'box' or 'action' are present this entry is considered a submenu
 * containing more entries.
 */
int menutree(const char *path, int toplevel)
{
	int ret;
	struct menu *menu;
	struct stat s;
	char *box;
	struct menutree *mt;
	glob_t g;
	int i;
	char *globpath, *display;
	size_t size;

	menu = menu_alloc();

	globpath = basprintf("%s/*", path);
	ret = glob(globpath, 0, NULL, &g);
	free(globpath);
	if (ret == GLOB_NOMATCH) {
		ret = -EINVAL;
		goto out;
	}

	globpath = basprintf("%s/title", path);
	display = read_file(globpath, &size);
	free(globpath);
	if (!display) {
		eprintf("no title found in %s/title\n", path);
		ret = -EINVAL;
		goto out;
	}

	strim(display);
	menu_add_title(menu, shell_expand(display));
	free(display);

	for (i = 0; i < g.gl_pathc; i++) {
		ret = stat(g.gl_pathv[i], &s);
		if (ret)
			goto out;

		if (!S_ISDIR(s.st_mode))
			continue;

		mt = xzalloc(sizeof(*mt));

		display = read_file_line("%s/title", g.gl_pathv[i]);
		if (!display) {
			eprintf("no title found in %s/title\n", g.gl_pathv[i]);
			ret = -EINVAL;
			goto out;
		}

		mt->me.display = shell_expand(display);
		free(display);
		mt->me.free = menutree_entry_free;

		box = read_file_line("%s/box", g.gl_pathv[i]);
		if (box) {
			mt->me.type = MENU_ENTRY_BOX;
			mt->me.action = menutree_box;
			mt->action = box;
			getenv_bool(box, &mt->me.box_state);
			menu_add_entry(menu, &mt->me);
			continue;
		}

		mt->me.type = MENU_ENTRY_NORMAL;

		mt->action = basprintf("%s/action", g.gl_pathv[i]);

		ret = stat(mt->action, &s);
		if (ret) {
			mt->me.action = menutree_action_subdir;
			free(mt->action);
			mt->action = xstrdup(g.gl_pathv[i]);
		} else {
			mt->me.action = menutree_action;
		}

		menu_add_entry(menu, &mt->me);
	}

	if (!toplevel) {
		mt = xzalloc(sizeof(*mt));
		mt->me.display = xstrdup("back");
		mt->me.type = MENU_ENTRY_NORMAL;
		mt->me.non_re_ent = 1;
		mt->me.free = menutree_entry_free;
		menu_add_entry(menu, &mt->me);
	}

	menu_show(menu);

	ret = 0;
out:
	menu_free(menu);

	globfree(&g);

	return ret;
}
