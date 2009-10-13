#include <common.h>
#include <xfuncs.h>
#include <malloc.h>
#include <stringlist.h>

int string_list_add(struct string_list *sl, char *str)
{
	struct string_list *new;

	new = xmalloc(sizeof(struct string_list) + strlen(str) + 1);

	strcpy(new->str, str);

	list_add_tail(&new->list, &sl->list);

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
	if (len == 0)
		len = 1;

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
