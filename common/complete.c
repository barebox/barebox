// SPDX-License-Identifier: GPL-2.0-only
/*
 * complete.c - functions for TAB completion
 *
 * Copyright (c) 2008 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <complete.h>
#include <xfuncs.h>
#include <fs.h>
#include <linux/stat.h>
#include <libgen.h>
#include <command.h>
#include <environment.h>

static bool is_valid_escape(const char *str)
{
	return str[0] == '\\' && (str[1] == ' ' || str[1] == '\\');
}

static bool strstarts_escaped(const char *whole, const char *prefix_escaped)
{
	if (!prefix_escaped)
		return true;

	while (*prefix_escaped) {
		if (is_valid_escape(prefix_escaped))
			prefix_escaped++;

		if (*whole++ != *prefix_escaped++)
			return false;
	}

	return true;
}

static int file_complete(struct string_list *sl, char *instr,
			 const char *dirn, int exec)
{
	char *path = strdup(instr);
	struct stat s;
	DIR *dir;
	struct dirent *d;
	char tmp[PATH_MAX];
	char *base;

	base = basename(instr);
	dirn = dirn ?: dirname(path);

	dir = opendir(dirn);
	if (!dir)
		goto out;

	while ((d = readdir(dir))) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		if (!strstarts_escaped(d->d_name, base))
			continue;

		strcpy(tmp, instr);
		strcat(tmp, d->d_name + strlen(base));
		if (!stat(tmp, &s) && S_ISDIR(s.st_mode)) {
			strcat(tmp, "/");
		} else {
			if (exec && !S_ISREG(s.st_mode))
				continue;
			strcat(tmp, " ");
		}

		string_list_add_sorted(sl, tmp);
	}

	closedir(dir);

out:
	free(path);

	return 0;
}

static int path_command_complete(struct string_list *sl, char *instr)
{
	struct stat s;
	DIR *dir;
	struct dirent *d;
	char tmp[PATH_MAX];
	char *path, *p, *n;

	p = path = strdup(getenv("PATH"));

	if (!path)
		return -1;

	while (p) {
		n = strchr(p, ':');
		if (n)
			*n++ = '\0';
		if (*p == '\0') {
			p = n;
			continue;
		}
		dir = opendir(p);

		/* We need to check all PATH dirs, so if one failed,
		 * try next */
		if (!dir) {
			p = n;
			continue;
		}

		while ((d = readdir(dir))) {
			if (!strcmp(d->d_name, ".") ||
					!strcmp(d->d_name, ".."))
				continue;

			if (strstarts_escaped(d->d_name, instr)) {
				strcpy(tmp, d->d_name);
				if (!stat(tmp, &s) &&
						S_ISDIR(s.st_mode))
					continue;
				else
					strcat(tmp, " ");

				string_list_add_sort_uniq(sl, tmp);
			}
		}

		closedir(dir);
		p = n;
	}

	free(path);

	return 0;
}

int command_complete(struct string_list *sl, char *instr)
{
	struct command *cmdtp;

	if (!instr)
		instr = "";

	for_each_command(cmdtp) {
		if (strncmp(instr, cmdtp->name, strlen(instr)))
			continue;

		string_list_add_asprintf(sl, "%s ", cmdtp->name);
	}

	return 0;
}
EXPORT_SYMBOL(command_complete);

int device_complete(struct string_list *sl, char *instr)
{
	struct device *dev;

	for_each_device(dev) {
		if (!strstarts_escaped(dev_name(dev), instr))
			continue;

		string_list_add_asprintf(sl, "%s ", dev_name(dev));
	}

	return COMPLETE_CONTINUE;
}
EXPORT_SYMBOL(device_complete);

static int device_param_complete(struct device *dev, const char *devname,
				 struct string_list *sl, char *instr, int eval)
{
	struct param_d *param;

	list_for_each_entry(param, &dev->parameters, list) {
		if (!strstarts_escaped(param->name, instr))
			continue;

		string_list_add_asprintf(sl, "%s%s.%s%c",
			eval ? "$" : "", devname, param->name,
			eval ? ' ' : '=');
	}

	return 0;
}

int driver_complete(struct string_list *sl, char *instr)
{
	struct driver_d *drv;

	for_each_driver(drv) {
		if (!strstarts_escaped(drv->name, instr))
			continue;

		string_list_add_asprintf(sl, "%s ", drv->name);
	}

	return COMPLETE_CONTINUE;
}
EXPORT_SYMBOL(driver_complete);

int empty_complete(struct string_list *sl, char *instr)
{
	return COMPLETE_END;
}
EXPORT_SYMBOL(empty_complete);

int command_var_complete(struct string_list *sl, char *instr)
{
	return COMPLETE_CONTINUE;
}
EXPORT_SYMBOL(command_var_complete);

int devicetree_alias_complete(struct string_list *sl, char *instr)
{
	struct device_node *aliases;
	struct property *p;

	aliases = of_find_node_by_path("/aliases");
	if (!aliases)
		return 0;

	list_for_each_entry(p, &aliases->properties, list) {
		if (strncmp(instr, p->name, strlen(instr)))
			continue;

		string_list_add_asprintf(sl, "%s ", p->name);
	}

	return 0;
}
EXPORT_SYMBOL(devicetree_alias_complete);

int devicetree_nodepath_complete(struct string_list *sl, char *instr)
{
	struct device_node *node, *child;
	char *dirn, *base;
	char *path = strdup(instr);

	if (*instr == '/') {
		dirn = dirname(path);
		base = basename(instr);
		node = of_find_node_by_path(dirn);
		if (!node)
			goto out;
	} else if (!*instr) {
		node = of_get_root_node();
		if (!node)
			goto out;
		base = "";
	} else {
		goto out;
	}

	for_each_child_of_node(node, child) {
		if (strncmp(base, child->name, strlen(base)))
			continue;
		string_list_add_asprintf(sl, "%pOF/", child);
	}
out:
	free(path);

	return 0;
}
EXPORT_SYMBOL(devicetree_nodepath_complete);

int devicetree_complete(struct string_list *sl, char *instr)
{
	devicetree_nodepath_complete(sl, instr);
	devicetree_alias_complete(sl, instr);

	return 0;
}
EXPORT_SYMBOL(devicetree_complete);

int devicetree_file_complete(struct string_list *sl, char *instr)
{
	devicetree_complete(sl, instr);
	file_complete(sl, instr, NULL, 0);

	return 0;
}
EXPORT_SYMBOL(devicetree_file_complete);

int tutorial_complete(struct string_list *sl, char *instr)
{
	file_complete(sl, instr, "/env/data/tutorial", 0);

	return 0;
}
EXPORT_SYMBOL(tutorial_complete);

static int env_param_complete(struct string_list *sl, char *instr, int eval)
{
	struct device *dev;
	struct variable_d *var;
	struct env_context *c;
	int len;
	char end = '=', *pos, *dot;
	char *begin = "";

	if (!instr)
		instr = "";

	if (eval) {
		begin = "$";
		end = ' ';
	}

	len = strlen(instr);

	c = get_current_context();
	list_for_each_entry(var, &c->local, list) {
		if (strncmp(instr, var_name(var), len))
			continue;
		string_list_add_asprintf(sl, "%s%s%c",
			begin, var_name(var), end);
	}

	c = get_current_context();
	while (c) {
		list_for_each_entry(var, &c->global, list) {
			if (strncmp(instr, var_name(var), len))
				continue;
			string_list_add_asprintf(sl, "%s%s%c",
				begin, var_name(var), end);
		}
		c = c->parent;
	}

	pos = instr;
	while ((dot = strchr(pos, '.'))) {
		char *devname;

		devname = xstrndup(instr, dot - instr);
		dev = get_device_by_name(devname);

		if (dev)
			device_param_complete(dev, devname, sl, dot + 1, eval);

		free(devname);
		pos = dot + 1;
	}

	len = strlen(instr);

	for_each_device(dev) {
		if (!strncmp(instr, dev_name(dev), len))
			device_param_complete(dev, dev_name(dev), sl, "", eval);
	}

	return 0;
}

int env_param_noeval_complete(struct string_list *sl, char *instr)
{
	return env_param_complete(sl, instr, 0);
}
EXPORT_SYMBOL(env_param_noeval_complete);

static int tab_pressed = 0;

void complete_reset(void)
{
	tab_pressed = 0;
}

static char *skip_to_last_unescaped_space(char *instr)
{
	char *t;

	t = strrchr(instr, ' ');
	if (t && (instr == t || t[-1] != '\\'))
		return t + 1;

	return instr;
}

static size_t strlen_escaped(char *instr)
{
	size_t count = 0;

	for (; *instr; instr++) {
		if (is_valid_escape(instr))
			instr++;

		count++;
	}

	return count;
}

static char* cmd_complete_lookup(struct string_list *sl, char *instr)
{
	struct command *cmdtp;
	int len;
	int ret = COMPLETE_END;
	char *res = NULL;

	for_each_command(cmdtp) {
		len = strlen(cmdtp->name);
		if (!strncmp(instr, cmdtp->name, len) && instr[len] == ' ') {
			instr += len + 1;
			instr = skip_to_last_unescaped_space(instr);

			if (cmdtp->complete) {
				ret = cmdtp->complete(sl, instr);
				res = instr;
			}
			goto end;
		}
	}

end:
	if (ret == COMPLETE_CONTINUE && *instr == '$')
		env_param_complete(sl, instr + 1, 1);

	return res;
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

	/* get the completion possibilities */
	instr = cmd_complete_lookup(&sl, t);
	if (!instr) {
		instr = t;
		if (t && (t[0] == '/' || !strncmp(t, "./", 2))) {
			file_complete(&sl, t, NULL, 1);
			instr = t;
		} else if ((t = strrchr(t, ' '))) {
			t++;
			file_complete(&sl, t, NULL, 0);
			instr = t;
		} else {
			command_complete(&sl, instr);
			path_command_complete(&sl, instr);
			env_param_complete(&sl, instr, 0);
		}
		if (*instr == '$')
			env_param_complete(&sl, instr + 1, 1);
	}

	pos = strlen_escaped(instr);

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
