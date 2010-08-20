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

static struct menu menus;

struct menu* menu_get_menus(void)
{
	return &menus;
}

void menu_free(struct menu *m)
{
	struct list_head *pos;
	struct menu_entry *me;

	if (!m)
		return;
	free(m->name);
	free(m->display);

	pos = &m->entries.list;

	if (pos->prev != pos->next && pos->prev != 0)
		list_for_each(pos, &m->entries.list) {
			me = list_entry(pos, struct menu_entry, list);
			menu_entry_free(me);
		}

	free(m);
}

int menu_add(struct menu *m)
{
	if (!m || !m->name)
		return -EINVAL;

	if (menu_get_by_name(m->name))
		return -EEXIST;

	list_add_tail(&m->list, &menus.list);

	m->nb_entries = 0;

	INIT_LIST_HEAD(&m->entries.list);

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
	list_add_tail(&me->list, &m->entries.list);

	return 0;
}

void menu_remove_entry(struct menu *m, struct menu_entry *me)
{
	struct list_head *pos;
	int i = 1;

	if (!m || !me)
		return;

	m->nb_entries--;
	list_del(&me->list);

	list_for_each(pos, &m->entries.list) {
		me = list_entry(pos, struct menu_entry, list);
		me->num = i++;
	}
}

struct menu* menu_get_by_name(char *name)
{
	struct list_head *pos;
	struct menu* m;

	if (!name)
		return NULL;

	list_for_each(pos, &menus.list) {
		m = list_entry(pos, struct menu, list);
		if(strcmp(m->name, name) == 0)
			return m;
	}

	return NULL;
}

struct menu_entry* menu_entry_get_by_num(struct menu* m, int num)
{
	struct list_head *pos;
	struct menu_entry* me;

	if (!m || num < 1 || num > m->nb_entries)
		return NULL;

	list_for_each(pos, &m->entries.list) {
		me = list_entry(pos, struct menu_entry, list);
		if(me->num == num)
			return me;
	}

	return NULL;
}

void menu_entry_free(struct menu_entry *me)
{
	if (!me)
		return;

	free(me->display);
	free(me);
}

static void print_menu_entry(struct menu *m, struct menu_entry *me, int reverse)
{
	gotoXY(me->num + 1, 3);
	if (reverse)
		printf_reverse("%d: %-*s", me->num, m->width, me->display);
	else
		printf("%d: %-*s", me->num, m->width, me->display);
}

int menu_set_selected_entry(struct menu *m, struct menu_entry* me)
{
	struct list_head *pos;
	struct menu_entry* tmp;

	if (!m || !me)
		return -EINVAL;

	list_for_each(pos, &m->entries.list) {
		tmp = list_entry(pos, struct menu_entry, list);
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

static void print_menu(struct menu *m)
{
	struct list_head *pos;
	struct menu_entry *me;

	clear();
	gotoXY(1, 2);
	if(m->display) {
		puts(m->display);
	} else {
		puts("Menu : ");
		puts(m->name);
	}

	list_for_each(pos, &m->entries.list) {
		me = list_entry(pos, struct menu_entry, list);
		if(m->selected != me)
			print_menu_entry(m, me, 0);
	}

	if (!m->selected) {
		m->selected = list_first_entry(&m->entries.list,
						struct menu_entry, list);
	}

	print_menu_entry(m, m->selected, 1);
}

int menu_show(struct menu *m)
{
	int ch;
	int escape = 0;

	if(!m || list_empty(&m->entries.list))
		return -EINVAL;

	print_menu(m);

	do {
		ch = getc();
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
			if (&(m->selected->list) == &(m->entries.list)) {
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
			if (&(m->selected->list) == &(m->entries.list)) {
				m->selected = list_entry(m->selected->list.next, struct menu_entry,
							 list);
			}
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

void menu_action_run(struct menu *m, struct menu_entry *me)
{
	int ret;
	const char *s = getenv((const char*)me->priv);

	/* can be a command as boot */
	if (!s)
		s = me->priv;

	ret = run_command (s, 0);

	if (ret < 0)
		udelay(1000000);
}

static int menu_init(void)
{
	INIT_LIST_HEAD(&menus.list);

	return 0;
}
postcore_initcall(menu_init);
