#include <common.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <stringlist.h>

static int string_list_compare(struct list_head *a, struct list_head *b)
{
	char *astr, *bstr;
	astr = (char *)list_entry(a, struct string_list, list)->str;
	bstr = (char *)list_entry(b, struct string_list, list)->str;

	return strcmp(astr, bstr);
}

int string_list_add(struct string_list *sl, char *str)
{
	struct string_list *new;

	new = xmalloc(sizeof(*new));
	new->str = xstrdup(str);

	list_add_tail(&new->list, &sl->list);

	return 0;
}

int string_list_add_asprintf(struct string_list *sl, const char *fmt, ...)
{
	struct string_list *new;
	va_list args;

	new = xmalloc(sizeof(*new));

	va_start(args, fmt);

	new->str = vasprintf(fmt, args);

	va_end(args);

	if (!new->str) {
		free(new);
		return -ENOMEM;
	}

	list_add_tail(&new->list, &sl->list);

	return 0;
}

int string_list_add_sorted(struct string_list *sl, char *str)
{
	struct string_list *new;

	new = xmalloc(sizeof(*new));
	new->str = xstrdup(str);

	list_add_sort(&new->list, &sl->list, string_list_compare);

	return 0;
}

int string_list_contains(struct string_list *sl, char *str)
{
	struct string_list *entry;

	list_for_each_entry(entry, &sl->list, list) {
		if (!strcmp(str, entry->str))
			return 1;
	}

	return 0;
}

void string_list_print_by_column(struct string_list *sl)
{
	int len = 0, num, i;
	struct string_list *entry;

	list_for_each_entry(entry, &sl->list, list) {
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
	list_for_each_entry(entry, &sl->list, list) {
		if (!(++i % num))
			printf("%s\n", entry->str);
		else
			printf("%-*s", len, entry->str);
	}
	if (i % num)
		printf("\n");
}
