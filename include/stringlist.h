/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STRINGLIST_H
#define __STRINGLIST_H

#include <linux/list.h>
#include <linux/string.h>
#include <malloc.h>

struct string_list {
	struct list_head list;
	const char *str;
};

int string_list_add(struct string_list *sl, const char *str);
int string_list_add_asprintf(struct string_list *sl, const char *fmt, ...)
	__printf(2, 3);
int string_list_add_sorted(struct string_list *sl, const char *str);
int string_list_add_sort_uniq(struct string_list *sl, const char *str);
int string_list_contains(struct string_list *sl, const char *str);
char *string_list_join(const struct string_list *sl, const char *joinstr);
void string_list_print_by_column(struct string_list *sl);

static inline void string_list_init(struct string_list *sl)
{
	INIT_LIST_HEAD(&sl->list);
	sl->str = NULL;
}

static inline size_t string_list_count(struct string_list *sl)
{
	return list_count_nodes(&sl->list);
}

static inline void string_list_free(struct string_list *sl)
{
	struct string_list *entry, *safe;

	list_for_each_entry_safe(entry, safe, &sl->list, list) {
		free_const(entry->str);
		free(entry);
	}
}

#define string_list_for_each_entry(entry, sl) \
	list_for_each_entry(entry, &(sl)->list, list)

#endif /* __STRINGLIST_H */
