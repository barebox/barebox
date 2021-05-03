/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SYSTEM_PARTITIONS_H_
#define SYSTEM_PARTITIONS_H_

#include <file-list.h>

#ifdef CONFIG_SYSTEM_PARTITIONS

/* duplicates current system_partitions and returns it */
struct file_list *system_partitions_get(void);

/* takes ownership of files and store it internally */
void system_partitions_set(struct file_list *files);

/*
 * check whether system_partitions_get would return an empty
 * file_list without doing an allocation
 */
bool system_partitions_empty(void);

#else

static inline struct file_list *system_partitions_get(void)
{
	return file_list_parse("");
}

static inline bool system_partitions_empty(void)
{
	return true;
}

/*
 * system_partitions_set() intentionally left unimplemented.
 * select CONFIG_SYSTEM_PARTITIONS if you want to set it
 */

#endif

#endif
