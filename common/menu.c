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
#include <environment.h>
#include <init.h>
#include <menu.h>
#include <malloc.h>
#include <xfuncs.h>
#include <errno.h>
#include <readkey.h>
#include <clock.h>
#include <linux/err.h>
#include <libbb.h>

static LIST_HEAD(menus);

struct menu* menu_get_menus(void)
{
	if (list_empty(&menus))
		return NULL;

	return list_entry(&menus, struct menu, list);
}
EXPORT_SYMBOL(menu_get_menus);

void menu_free(struct menu *m)
{
	struct menu_entry *me, *tmp;
	int i;

	if (!m)
		return;
	free(m->name);
	for (i = 0; i < m->display_lines; i++)
		free(m->display[i]);
	free(m->display);
	free(m->auto_display);

	list_for_each_entry_safe(me, tmp, &m->entries, list)
		menu_entry_free(me);

	free(m);
}
EXPORT_SYMBOL(menu_free);

int menu_add(struct menu *m)
{
	if (!m || !m->name)
		return -EINVAL;

	if (menu_get_by_name(m->name))
		return -EEXIST;

	list_add_tail(&m->list, &menus);

	return 0;
}
EXPORT_SYMBOL(menu_add);

void menu_remove(struct menu *m)
{
	if (!m)
		return;

	list_del(&m->list);
}
EXPORT_SYMBOL(menu_remove);

int menu_add_entry(struct menu *m, struct menu_entry *me)
{
	if (!m || !me || !me->display)
		return -EINVAL;

	m->nb_entries++;
	me->num = m->nb_entries;
	list_add_tail(&me->list, &m->entries);

	return 0;
}
EXPORT_SYMBOL(menu_add_entry);

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
EXPORT_SYMBOL(menu_remove_entry);

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
EXPORT_SYMBOL(menu_get_by_name);

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
EXPORT_SYMBOL(menu_entry_get_by_num);

void menu_entry_free(struct menu_entry *me)
{
	if (!me)
		return;

	me->free(me);
}
EXPORT_SYMBOL(menu_entry_free);

static void __print_entry(const char *str)
{
	static char outstr[256];

	if (IS_ENABLED(CONFIG_SHELL_HUSH)) {
		process_escape_sequence(str, outstr, 256);
		puts(outstr);
	} else {
		puts(str);
	}
}

static void print_menu_entry(struct menu *m, struct menu_entry *me,
			     int selected)
{
	gotoXY(3, me->num + m->display_lines);

	if (me->type == MENU_ENTRY_BOX) {
		if (me->box_state)
			puts("[*]");
		else
			puts("[ ]");
	} else {
		puts("   ");
	}

	printf(" %2d: ", me->num);
	if (selected)
		puts("\e[7m");

	__print_entry(me->display);

	if (selected)
		puts("\e[0m");
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
EXPORT_SYMBOL(menu_set_selected_entry);

int menu_set_selected(struct menu *m, int num)
{
	struct menu_entry *me;

	me = menu_entry_get_by_num(m, num);

	if (!me)
		return -EINVAL;

	m->selected = me;

	return 0;
}
EXPORT_SYMBOL(menu_set_selected);

int menu_set_auto_select(struct menu *m, int delay)
{
	if (!m)
		return -EINVAL;

	m->auto_select = delay;

	return 0;
}
EXPORT_SYMBOL(menu_set_auto_select);

static void print_menu(struct menu *m)
{
	struct menu_entry *me;
	int i;

	clear();
	for (i = 0; i < m->display_lines; i++) {
		gotoXY(2, 1 + i);
		__print_entry(m->display[i]);
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
	int ch, ch_previous = 0;
	int countdown;
	int auto_display_len = 16;
	uint64_t start, second;

	if(!m || list_empty(&m->entries))
		return -EINVAL;

	print_menu(m);

	countdown = m->auto_select;
	if (m->auto_select >= 0) {
		gotoXY(3, m->nb_entries + m->display_lines + 1);
		if (!m->auto_display) {
			printf("Auto Select in");
		} else {
			auto_display_len = strlen(m->auto_display);
			printf("%s", m->auto_display);
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

	gotoXY(3, m->nb_entries + m->display_lines + 1);
	printf("%*c", auto_display_len + 4, ' ');

	gotoXY(3, m->selected->num + m->display_lines);

	do {
		struct menu_entry *old_selected = m->selected;
		int repaint = 0;

		if (m->auto_select >= 0)
			ch = BB_KEY_RETURN;
		else
			ch = read_key();

		m->auto_select = -1;

		switch (ch) {
		case '0' ... '9': {
			struct menu_entry *me;
			int num = ch - '0';
			int next_num = m->selected->num + 10;
			if (!num)
				num = 10;

			if (ch_previous == ch && next_num <= m->nb_entries)
				num = next_num;

			me = menu_entry_get_by_num(m, num);
			if (me) {
				m->selected = me;
				repaint = 1;
			}
			break;
		}
		case 'k':
		case BB_KEY_UP:
			m->selected = list_entry(m->selected->list.prev, struct menu_entry,
						 list);
			if (&(m->selected->list) == &(m->entries)) {
				m->selected = list_entry(m->selected->list.prev, struct menu_entry,
							 list);
			}
			repaint = 1;
			break;
		case 'j':
		case BB_KEY_DOWN:
			m->selected = list_entry(m->selected->list.next, struct menu_entry,
						 list);
			if (&(m->selected->list) == &(m->entries)) {
				m->selected = list_entry(m->selected->list.next, struct menu_entry,
							 list);
			}
			repaint = 1;
			break;
		case ' ':
			if (m->selected->type == MENU_ENTRY_BOX) {
				m->selected->box_state = !m->selected->box_state;
				if (m->selected->action)
					m->selected->action(m, m->selected);
				repaint = 1;
				break;
			}
			/* no break */
		case BB_KEY_ENTER:
			if (ch_previous == BB_KEY_RETURN)
				break;
		case BB_KEY_RETURN:
			if (ch_previous == BB_KEY_ENTER)
				break;
			clear();
			gotoXY(1,1);
			if (m->selected->action)
				m->selected->action(m, m->selected);
			if (m->selected->non_re_ent)
				return m->selected->num;
			else
				print_menu(m);
			break;
		case BB_KEY_HOME:
			m->selected = list_first_entry(&m->entries, struct menu_entry, list);
			repaint = 1;
			break;
		case BB_KEY_END:
			m->selected = list_last_entry(&m->entries, struct menu_entry, list);
			repaint = 1;
			break;
		default:
			break;
		}

		if (repaint) {
			print_menu_entry(m, old_selected, 0);
			print_menu_entry(m, m->selected, 1);
		}

		ch_previous = ch;
	} while(1);

	return 0;
}
EXPORT_SYMBOL(menu_show);

void menu_action_exit(struct menu *m, struct menu_entry *me) {}
EXPORT_SYMBOL(menu_action_exit);

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
EXPORT_SYMBOL(menu_add_submenu);

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

	ret = run_command(s);

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
EXPORT_SYMBOL(menu_add_command_entry);

/*
 * Add title to menu.
 * Lines are separated by explicit char '\n' or by string "\n".
 *
 * @display: NULL or pointer to the string which will be freed in this function.
 *	If NULL or zero length string is provided, default title will be added.
 */
void menu_add_title(struct menu *m, const char *display)
{
	char *tmp, *src, *dst;
	int lines = 1;
	int i;

	if (!display || !strlen(display))
		src = dst = tmp = xasprintf("Menu : %s", m->name ? m->name : "");
	else
		src = dst = tmp = xstrdup(display);

	/* Count lines and separate single string into multiple strings */
	while (*src) {
		if (*src == '\\') {
			if (*(src + 1) == '\\') {
				*dst++ = *src++;
				*dst++ = *src++;
				continue;
			}
			if (*(src + 1) == 'n') {
				*dst = 0;
				src += 2;
				dst++;
				lines++;
				continue;
			}
		}
		if (*src == '\n') {
			*dst = 0;
			src++;
			dst++;
			lines++;
			continue;
		}
		*dst++ = *src++;
	}
	*dst = 0;

	m->display = xzalloc(sizeof(*m->display) * lines);
	m->display_lines = lines;

	for (src = tmp, i = 0; i < lines; i++) {
		m->display[i] = xstrdup(src);
		/* Go to the next line */
		src += strlen(src) + 1;
	}

	free(tmp);
}
EXPORT_SYMBOL(menu_add_title);
