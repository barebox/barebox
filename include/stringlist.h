#ifndef __STRINGLIST_H
#define __STRINGLIST_H

#include <linux/list.h>

struct string_list {
	struct list_head list;
	char *str;
};

int string_list_add(struct string_list *sl, char *str);
int string_list_add_asprintf(struct string_list *sl, const char *fmt, ...);
int string_list_add_sorted(struct string_list *sl, char *str);
int string_list_contains(struct string_list *sl, char *str);
void string_list_print_by_column(struct string_list *sl);

static inline void string_list_init(struct string_list *sl)
{
	INIT_LIST_HEAD(&sl->list);
}

static inline void string_list_free(struct string_list *sl)
{
	struct string_list *entry, *safe;

	list_for_each_entry_safe(entry, safe, &sl->list, list) {
		free(entry->str);
		free(entry);
	}
}

#endif /* __STRINGLIST_H */
