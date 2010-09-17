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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <init.h>
#include <menu.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <readkey.h>
#include <clock.h>
#include <linux/err.h>

static LIST_HEAD(menus);

struct menu* menu_get_menus(void)
{
	if (list_empty(&menus))
		return NULL;

	return list_entry(&menus, struct menu, list);
}

void menu_free(struct menu *m)
{
	struct menu_entry *me, *tmp;

	if (!m)
		return;
	free(m->name);
	free(m->display);
	free(m->auto_display);

	list_for_each_entry_safe(me, tmp, &m->entries, list)
		menu_entry_free(me);

	free(m);
}

int menu_add(struct menu *m)
{
	if (!m || !m->name)
		return -EINVAL;

	if (menu_get_by_name(m->name))
		return -EEXIST;

	list_add_tail(&m->list, &menus);

	return 0;
}

void menu_remove(struct menu *m)
{
	if (!m)
		return;

	list_del(&m->list);
}

int menu_add_entry(struct menu *m, struct menu_entry *me)
{
	int len;

	if (!m || !me || !me->display)
		return -EINVAL;

	len = strlen(me->display);

	m->width = max(len, m->width);

	m->nb_entries++;
	me->num = m->nb_entries;
	list_add_tail(&me->list, &m->entries);

	return 0;
}

void menu_remove_entry(struct menu *m, struct menu_entry *me)
{
	int i = 1;

	if (!m || !me)
		return;

	m->nb_entries--;
	list_del(&me->list);

	list_for_each_entry(me, &m->entries, list)
		me->num = i++;
}

struct menu* menu_get_by_name(char *name)
{
	struct menu* m;

	if (!name)
		return NULL;

	list_for_each_entry(m, &menus, list) {
		if(strcmp(m->name, name) == 0)
			return m;
	}

	return NULL;
}

struct menu_entry* menu_entry_get_by_num(struct menu* m, int num)
{
	struct menu_entry* me;

	if (!m || num < 1 || num > m->nb_entries)
		return NULL;

	list_for_each_entry(me, &m->entries, list) {
		if(me->num == num)
			return me;
	}

	return NULL;
}

void menu_entry_free(struct menu_entry *me)
{
	if (!me)
		return;

	me->free(me);
}

static void print_menu_entry(struct menu *m, struct menu_entry *me,
			     int selected)
{
	gotoXY(me->num + 1, 3);
	if (selected)
		printf("\e[7m");

	if (me->type == MENU_ENTRY_BOX) {
		if (me->box_state)
			puts("[*]");
		else
			puts("[ ]");
	} else {
		puts("   ");
	}

	printf(" %d: %-*s", me->num, m->width, me->display);

	if (selected)
		printf("\e[m");
}

int menu_set_selected_entry(struct menu *m, struct menu_entry* me)
{
	struct menu_entry* tmp;

	if (!m || !me)
		return -EINVAL;

	list_for_each_entry(tmp, &m->entries, list) {
		if(me == tmp) {
			m->selected = me;
			return 0;
		}
	}

	return -EINVAL;
}

int menu_set_selected(struct menu *m, int num)
{
	struct menu_entry *me;

	me = menu_entry_get_by_num(m, num);

	if (!me)
		return -EINVAL;

	m->selected = me;

	return 0;
}

int menu_set_auto_select(struct menu *m, int delay)
{
	if (!m)
		return -EINVAL;

	m->auto_select = delay;

	return 0;
}

static void print_menu(struct menu *m)
{
	struct menu_entry *me;

	clear();
	gotoXY(1, 2);
	if(m->display) {
		puts(m->display);
	} else {
		puts("Menu : ");
		puts(m->name);
	}

	list_for_each_entry(me, &m->entries, list) {
		if(m->selected != me)
			print_menu_entry(m, me, 0);
	}

	if (!m->selected) {
		m->selected = list_first_entry(&m->entries,
						struct menu_entry, list);
	}

	print_menu_entry(m, m->selected, 1);
}

int menu_show(struct menu *m)
{
	int ch;
	int escape = 0;
	int countdown;
	int auto_display_len = 16;
	uint64_t start, second;

	if(!m || list_empty(&m->entries))
		return -EINVAL;

	print_menu(m);

	countdown = m->auto_select;
	if (m->auto_select >= 0) {
		gotoXY(m->nb_entries + 2, 3);
		if (!m->auto_display) {
			printf("Auto Select in");
		} else {
			auto_display_len = strlen(m->auto_display);
			printf(m->auto_display);
		}
		printf(" %2d", countdown--);
	}

	start = get_time_ns();
	second = start;
	while (m->auto_select > 0 && !is_timeout(start, m->auto_select * SECOND)) {
		if (tstc()) {
			m->auto_select = -1;
			break;
		}

		if (is_timeout(second, SECOND)) {
			printf("\b\b%2d", countdown--);
			second += SECOND;
		}
	}

	gotoXY(m->nb_entries + 2, 3);
	printf("%*c", auto_display_len + 4, ' ');

	gotoXY(m->selected->num + 1, 3);

	do {
		if (m->auto_select >= 0)
			ch = '\n';
		else
			ch = getc();

		m->auto_select = -1;

		switch(ch) {
		case 0x1b:
			escape = 1;
			break;
		case '[':
			if (escape)
				break;
		case 'A': /* up */
			escape = 0;
			print_menu_entry(m, m->selected, 0);
			m->selected = list_entry(m->selected->list.prev, struct menu_entry,
						 list);
			if (&(m->selected->list) == &(m->entries)) {
				m->selected = list_entry(m->selected->list.prev, struct menu_entry,
							 list);
			}
			print_menu_entry(m, m->selected, 1);
			break;
		case 'B': /* down */
			escape = 0;
			print_menu_entry(m, m->selected, 0);
			m->selected = list_entry(m->selected->list.next, struct menu_entry,
						 list);
			if (&(m->selected->list) == &(m->entries)) {
				m->selected = list_entry(m->selected->list.next, struct menu_entry,
							 list);
			}
			print_menu_entry(m, m->selected, 1);
			break;
		case ' ':
			if (m->selected->type != MENU_ENTRY_BOX)
				break;
			m->selected->box_state = !m->selected->box_state;
			if (m->selected->action)
				m->selected->action(m, m->selected);
			print_menu_entry(m, m->selected, 1);
			break;
		case '\n':
		case '\r':
			clear();
			gotoXY(1,1);
			m->selected->action(m, m->selected);
			if (m->selected->non_re_ent)
				return m->selected->num;
			else
				print_menu(m);
		default:
			break;
		}
	} while(1);

	return 0;
}

void menu_action_exit(struct menu *m, struct menu_entry *me) {}

struct submenu {
	char *submenu;
	struct menu_entry entry;
};

static void menu_action_show(struct menu *m, struct menu_entry *me)
{
	struct submenu *s = container_of(me, struct submenu, entry);
	struct menu *sm;

	if (me->type == MENU_ENTRY_BOX && !me->box_state)
		return;

	sm = menu_get_by_name(s->submenu);
	if (sm)
		menu_show(sm);
	else
		eprintf("no such menu: %s\n", s->submenu);
}

static void submenu_free(struct menu_entry *me)
{
	struct submenu *s = container_of(me, struct submenu, entry);

	free(s->entry.display);
	free(s->submenu);
	free(s);
}

struct menu_entry *menu_add_submenu(struct menu *parent, char *submenu, char *display)
{
	struct submenu *s = calloc(1, sizeof(*s));
	int ret;

	if (!s)
		return ERR_PTR(-ENOMEM);

	s->submenu = strdup(submenu);
	s->entry.action = menu_action_show;
	s->entry.free = submenu_free;
	s->entry.display = strdup(display);
	if (!s->entry.display || !s->submenu) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = menu_add_entry(parent, &s->entry);
	if (ret)
		goto err_free;

	return &s->entry;

err_free:
	submenu_free(&s->entry);
	return ERR_PTR(ret);
}

struct action_entry {
	char *command;
	struct menu_entry entry;
};

static void menu_action_command(struct menu *m, struct menu_entry *me)
{
	struct action_entry *e = container_of(me, struct action_entry, entry);
	int ret;
	const char *s = getenv(e->command);

	/* can be a command as boot */
	if (!s)
		s = e->command;

	ret = run_command (s, 0);

	if (ret < 0)
		udelay(1000000);
}

static void menu_command_free(struct menu_entry *me)
{
	struct action_entry *e = container_of(me, struct action_entry, entry);

	free(e->entry.display);
	free(e->command);

	free(e);
}

struct menu_entry *menu_add_command_entry(struct menu *m, char *display,
					  char *command, menu_entry_type type)
{
	struct action_entry *e = calloc(1, sizeof(*e));
	int ret;

	if (!e)
		return ERR_PTR(-ENOMEM);

	e->command = strdup(command);
	e->entry.action = menu_action_command;
	e->entry.free = menu_command_free;
	e->entry.type = type;
	e->entry.display = strdup(display);

	if (!e->entry.display || !e->command) {
		ret = -ENOMEM;
		goto err_free;
	}

	ret = menu_add_entry(m, &e->entry);
	if (ret)
		goto err_free;

	return &e->entry;
err_free:
	menu_command_free(&e->entry);
	return ERR_PTR(ret);
}

