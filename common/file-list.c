// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt)	"file_list: " fmt

#include <common.h>
#include <malloc.h>
#include <fs.h>
#include <file-list.h>
#include <stringlist.h>
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

static const char *flags_to_str(int flags)
{
	static char str[sizeof "srcu"];
	char *s = str;;

	if (flags & FILE_LIST_FLAG_SAFE)
		*s++ = 's';
	if (flags & FILE_LIST_FLAG_READBACK)
		*s++ = 'r';
	if (flags & FILE_LIST_FLAG_CREATE)
		*s++ = 'c';
	if (flags & FILE_LIST_FLAG_UBI)
		*s++ = 'u';

	*s = '\0';

	return str;
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

	if (!files)
		return;

	list_for_each_entry_safe(entry, tmp, &files->list, list) {
		free(entry->name);
		free(entry->filename);
		free(entry);
	}

	free(files);
}

struct file_list *file_list_dup(struct file_list *old)
{
	struct file_list_entry *old_entry;
	struct file_list *new;

	new = xzalloc(sizeof(*new));

	INIT_LIST_HEAD(&new->list);

	list_for_each_entry(old_entry, &old->list, list) {
		(void)file_list_add_entry(new, old_entry->name, old_entry->filename,
					  old_entry->flags); /* can't fail */
		new->num_entries++;
	}

	return new;
}

char *file_list_to_str(const struct file_list *files)
{
	struct file_list_entry *entry;
	struct string_list sl;
	char *str;

	if (!files)
		return strdup("");

	string_list_init(&sl);

	list_for_each_entry(entry, &files->list, list) {
		int ret = string_list_add_asprintf(&sl, "%s(%s)%s", entry->filename, entry->name,
						   flags_to_str(entry->flags));
		if (ret) {
			str = ERR_PTR(ret);
			goto out;
		}
	}

	str = string_list_join(&sl, ",");
out:
	string_list_free(&sl);

	return str;
}
