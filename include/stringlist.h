#ifndef __STRING_H
#define __STRING_H

#include <linux/list.h>

struct string_list {
	struct list_head list;
	char str[0];
};

int string_list_add(struct string_list *sl, char *str);
void string_list_print_by_column(struct string_list *sl);

static inline void string_list_init(struct string_list *sl)
{
	INIT_LIST_HEAD(&sl->list);
}

static inline void string_list_free(struct string_list *sl)
{
	struct string_list *entry, *safe;

	list_for_each_entry_safe(entry, safe, &sl->list, list)
		free(entry);
}

#endif /* __STRING_H */
