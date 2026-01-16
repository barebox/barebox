// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <stringlist.h>

static int string_list_compare(struct list_head *a, struct list_head *b)
{
	char *astr, *bstr;
	astr = (char *)list_entry(a, struct string_list, list)->str;
	bstr = (char *)list_entry(b, struct string_list, list)->str;

	return strcmp(astr, bstr);
}

int string_list_add(struct string_list *sl, const char *str)
{
	struct string_list *new;

	new = xmalloc(sizeof(*new));
	new->str = xstrdup_const(str);

	list_add_tail(&new->list, &sl->list);

	return 0;
}
EXPORT_SYMBOL(string_list_add);

int string_list_add_asprintf(struct string_list *sl, const char *fmt, ...)
{
	struct string_list *new;
	va_list args;

	new = xmalloc(sizeof(*new));

	va_start(args, fmt);

	new->str = bvasprintf(fmt, args);

	va_end(args);

	if (!new->str) {
		free(new);
		return -ENOMEM;
	}

	list_add_tail(&new->list, &sl->list);

	return 0;
}
EXPORT_SYMBOL(string_list_add_asprintf);

int string_list_add_sorted(struct string_list *sl, const char *str)
{
	struct string_list *new;

	new = xmalloc(sizeof(*new));
	new->str = xstrdup_const(str);

	list_add_sort(&new->list, &sl->list, string_list_compare);

	return 0;
}

int string_list_add_sort_uniq(struct string_list *sl, const char *str)
{
	struct string_list *new, *entry = sl;

	string_list_for_each_entry(entry, sl) {
		int cmp = strcmp(entry->str, str);

		if (cmp < 0)
			continue;
		if (cmp == 0)
			return 0;

		break;
	}

	new = xmalloc(sizeof(*new));
	new->str = xstrdup_const(str);

	list_add_tail(&new->list, &entry->list);

	return 0;
}

int string_list_contains(struct string_list *sl, const char *str)
{
	struct string_list *entry;

	string_list_for_each_entry(entry, sl) {
		if (!strcmp(str, entry->str))
			return 1;
	}

	return 0;
}

char *string_list_join(const struct string_list *sl, const char *joinstr)
{
	struct string_list *entry;
	size_t len = 0;
	size_t joinstr_len = strlen(joinstr);
	char *str, *next;

	string_list_for_each_entry(entry, sl)
		len += strlen(entry->str) + joinstr_len;

	if (len == 0)
		return strdup("");

	str = malloc(len + 1);
	if (!str)
		return NULL;

	next = str;

	string_list_for_each_entry(entry, sl) {
		next = stpcpy(next, entry->str);
		next = stpcpy(next, joinstr);
	}

	next[-joinstr_len] = '\0';

	return str;
}
EXPORT_SYMBOL(string_list_join);

void string_list_print_by_column(struct string_list *sl)
{
	int len = 0, num, i;
	struct string_list *entry;

	string_list_for_each_entry(entry, sl) {
		int l = strlen(entry->str) + 4;
		if (l > len)
			len = l;
	}

	if (!len)
		return;

	num = 80 / (len + 1);
	if (num == 0)
		num = 1;

	i = 0;
	string_list_for_each_entry(entry, sl) {
		if (!(++i % num))
			printf("%s\n", entry->str);
		else
			printf("%-*s", len, entry->str);
	}
	if (i % num)
		printf("\n");
}
