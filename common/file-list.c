#include <common.h>
#include <malloc.h>
#include <fs.h>
#include <file-list.h>
#include <linux/err.h>

#define PARSE_DEVICE	0
#define PARSE_NAME	1
#define PARSE_FLAGS	2

static int file_list_parse_one(struct file_list *files, const char *partstr, const char **endstr)
{
	int i = 0, state = PARSE_DEVICE;
	char filename[PATH_MAX];
	char name[PATH_MAX];
	struct file_list_entry *entry = xzalloc(sizeof(*entry));

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
				entry->flags |= FILE_LIST_FLAG_SAFE;
				break;
			case 'r':
				entry->flags |= FILE_LIST_FLAG_READBACK;
				break;
			case 'c':
				entry->flags |= FILE_LIST_FLAG_CREATE;
				break;
			default:
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
		partstr++;
	}

	if (state != PARSE_FLAGS)
		return -EINVAL;

	entry->name = xstrdup(name);
	entry->filename = xstrdup(filename);
	if (*partstr == ',')
		partstr++;
	*endstr = partstr;

	list_add_tail(&entry->list, &files->list);

	return 0;
}

struct file_list *file_list_parse(const char *str)
{
	struct file_list *files;
	int ret;
	const char *endptr;

	files = xzalloc(sizeof(*files));

	INIT_LIST_HEAD(&files->list);

	while (*str) {
		if (file_list_parse_one(files, str, &endptr)) {
			printf("parse error\n");
			ret = -EINVAL;
			goto out;
		}
		str = endptr;

		files->num_entries++;
	}

	return files;
out:
	free(files);

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
