/*
 * complete.c - functions for TAB completion
 *
 * Copyright (c) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <complete.h>
#include <xfuncs.h>
#include <linux/list.h>
#include <malloc.h>
#include <fs.h>
#include <linux/stat.h>
#include <libgen.h>
#include <command.h>
#include <stringlist.h>

static int file_complete(struct string_list *sl, char *instr)
{
	char *path = strdup(instr);
	struct stat s;
	DIR *dir;
	struct dirent *d;
	char tmp[PATH_MAX];
	char *base, *dirn;

	base = basename(instr);
	dirn = dirname(path);

	dir = opendir(dirn);
	if (!dir)
		goto out;

	while ((d = readdir(dir))) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		if (!strncmp(base, d->d_name, strlen(base))) {
			strcpy(tmp, instr);
			strcat(tmp, d->d_name + strlen(base));
			if (!stat(tmp, &s) && S_ISDIR(s.st_mode))
				strcat(tmp, "/");
			else
				strcat(tmp, " ");
			string_list_add(sl, tmp);
		}
	}

	closedir(dir);

out:
	free(path);

	return 0;
}

static int command_complete(struct string_list *sl, char *instr)
{
	struct command *cmdtp;
	char cmd[128];

	for_each_command(cmdtp) {
		if (!strncmp(instr, cmdtp->name, strlen(instr))) {
			strcpy(cmd, cmdtp->name);
			cmd[strlen(cmdtp->name)] = ' ';
			cmd[strlen(cmdtp->name) + 1] = 0;
			string_list_add(sl, cmd);
		}
	}

	return 0;
}

static int tab_pressed = 0;

void complete_reset(void)
{
	tab_pressed = 0;
}

int complete(char *instr, char **outstr)
{
	struct string_list sl, *entry, *first_entry;
	int pos;
	char ch;
	int changed;
	static char out[256];
	int outpos = 0;
	int reprint = 0;
	char *t;

	string_list_init(&sl);

	/* advance to the last command */
	t = strrchr(instr, ';');
	if (!t)
		t = instr;
	else
		t++;

	while (*t == ' ')
		t++;

	instr = t;

	/* get the completion possibilities */
	if ((t = strrchr(t, ' '))) {
		t++;
		file_complete(&sl, t);
		instr = t;
	} else
		command_complete(&sl, instr);

	pos = strlen(instr);

	*outstr = "";
	if (list_empty(&sl.list))
		return reprint;

	out[0] = 0;

	first_entry = list_first_entry(&sl.list, struct string_list, list);

	while (1) {
		entry = first_entry;
		ch = entry->str[pos];
		if (!ch)
			break;

		changed = 0;
		list_for_each_entry(entry, &sl.list, list) {
			if (!entry->str[pos])
				break;
			if (ch != entry->str[pos]) {
				changed = 1;
				break;
			}
		}

		if (changed)
			break;
		out[outpos++] = ch;
		pos++;
	}

	if (!list_is_last(&first_entry->list, &sl.list) && !outpos && tab_pressed) {
		printf("\n");
		string_list_print_by_column(&sl);
		reprint = 1;
	}

	out[outpos++] = 0;
	*outstr = out;

	if (*out == 0)
		tab_pressed = 1;
	else
		tab_pressed = 0;

	string_list_free(&sl);

	return reprint;
}

