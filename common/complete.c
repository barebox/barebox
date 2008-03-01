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
#include <list.h>
#include <malloc.h>
#include <fs.h>
#include <linux/stat.h>
#include <libgen.h>
#include <command.h>

static int complete_push(struct complete_handle *handle, char *str)
{
	struct complete_handle *new;

	new = xmalloc(sizeof(struct complete_handle) + strlen(str) + 1);

	strcpy(new->str, str);

	list_add_tail(&new->list, &handle->list);

	return 0;
}

static int file_complete(struct complete_handle *handle, char *instr)
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
			complete_push(handle, tmp);
		}
	}

	closedir(dir);

out:
	free(path);

	return 0;
}

static int command_complete(struct complete_handle *handle, char *instr)
{
	cmd_tbl_t *cmdtp;
	char cmd[128];

	for_each_command(cmdtp) {
		if (!strncmp(instr, cmdtp->name, strlen(instr))) {
			strcpy(cmd, cmdtp->name);
			cmd[strlen(cmdtp->name)] = ' ';
			cmd[strlen(cmdtp->name) + 1] = 0;
			complete_push(handle, cmd);
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
	struct complete_handle c, *entry, *first_entry, *safe;
	int pos;
	char ch;
	int changed;
	static char out[256];
	int outpos = 0;
	int reprint = 0;
	char *t;

	INIT_LIST_HEAD(&c.list);

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
		file_complete(&c, t);
		instr = t;
	} else
		command_complete(&c, instr);

	pos = strlen(instr);

	*outstr = "";
	if (list_empty(&c.list))
		return reprint;

	out[0] = 0;

	first_entry = list_first_entry(&c.list, struct complete_handle, list);

	while (1) {
		entry = first_entry;
		ch = entry->str[pos];
		if (!ch)
			break;

		changed = 0;
		list_for_each_entry(entry, &c.list, list) {
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

	if (!list_is_last(&first_entry->list, &c.list) && !outpos && tab_pressed) {
		int len = 0, num, i;

		printf("\n");

		list_for_each_entry(entry, &c.list, list) {
			int l = strlen(entry->str) + 4;
			if (l > len)
				len = l;
		}

		num = 80 / len;
		if (len == 0)
			len = 1;

		i = 0;
		list_for_each_entry(entry, &c.list, list) {
			printf("%-*s    ", len, entry->str);
			if (!(++i % num))
				printf("\n");
		}
		if (i % num)
			printf("\n");
		reprint = 1;
	}

	out[outpos++] = 0;
	*outstr = out;

	if (*out == 0)
		tab_pressed = 1;
	else
		tab_pressed = 0;

	list_for_each_entry_safe(entry, safe, &c.list, list)
		free(entry);

	return reprint;
}

