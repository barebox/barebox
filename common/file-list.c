/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#define pr_fmt(fmt)	"file_list: " fmt

#include <common.h>
#include <malloc.h>
#include <fs.h>
#include <file-list.h>
#include <linux/err.h>

#define PARSE_DEVICE	0
#define PARSE_NAME	1
#define PARSE_FLAGS	2

struct file_list_entry *file_list_entry_by_name(struct file_list *files, const char *name)
{
	struct file_list_entry *entry;

	file_list_for_each_entry(files, entry) {
		if (!strcmp(entry->name, name))
			return entry;
	}

	return NULL;
}

int file_list_add_entry(struct file_list *files, const char *name, const char *filename,
			unsigned long flags)
{
	struct file_list_entry *entry;

	entry = file_list_entry_by_name(files, name);
	if (entry)
		return -EEXIST;

	entry = xzalloc(sizeof(*entry));

	entry->name = xstrdup(name);
	entry->filename = xstrdup(filename);
	entry->flags = flags;

	list_add_tail(&entry->list, &files->list);

	return 0;
}

static int file_list_parse_one(struct file_list *files, const char *partstr, const char **endstr)
{
	int i = 0, state = PARSE_DEVICE;
	char filename[PATH_MAX];
	char name[PATH_MAX];
	unsigned long flags = 0;

	memset(filename, 0, sizeof(filename));
	memset(name, 0, sizeof(name));

	while (*partstr && *partstr != ',') {
		switch (state) {
		case PARSE_DEVICE:
			if (*partstr == '(') {
				state = PARSE_NAME;
				i = 0;
			} else {
				filename[i++] = *partstr;
			}
			break;
		case PARSE_NAME:
			if (*partstr == ')') {
				state = PARSE_FLAGS;
				i = 0;
			} else {
				name[i++] = *partstr;
			}
			break;
		case PARSE_FLAGS:
			switch (*partstr) {
			case 's':
				flags |= FILE_LIST_FLAG_SAFE;
				break;
			case 'r':
				flags |= FILE_LIST_FLAG_READBACK;
				break;
			case 'c':
				flags |= FILE_LIST_FLAG_CREATE;
				break;
			case 'u':
				flags |= FILE_LIST_FLAG_UBI;
				break;
			default:
				pr_err("Unknown flag '%c'\n", *partstr);
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
		partstr++;
	}

	if (state != PARSE_FLAGS) {
		pr_err("Missing ')'\n");
		return -EINVAL;
	}

	if (*partstr == ',')
		partstr++;
	*endstr = partstr;

	return file_list_add_entry(files, name, filename, flags);
}

struct file_list *file_list_parse(const char *str)
{
	struct file_list *files;
	int ret;
	const char *endptr;

	files = xzalloc(sizeof(*files));

	INIT_LIST_HEAD(&files->list);

	while (*str) {
		ret = file_list_parse_one(files, str, &endptr);
		if (ret) {
			pr_err("parse error\n");
			goto out;
		}
		str = endptr;

		files->num_entries++;
	}

	return files;
out:
	file_list_free(files);

	return ERR_PTR(ret);
}

void file_list_free(struct file_list *files)
{
	struct file_list_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &files->list, list) {
		free(entry->name);
		free(entry->filename);
		free(entry);
	}

	free(files);
}
