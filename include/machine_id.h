/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACHINE_ID_H__
#define __MACHINE_ID_H__

#include <linux/types.h>
#include <linux/uuid.h>

#if IS_ENABLED(CONFIG_MACHINE_ID)

const void *machine_id_get_hashable(size_t *len);

void machine_id_set_hashable(const void *hashable, size_t len);
int machine_id_get_app_specific(uuid_t *result, ...) __attribute__((__sentinel__));

#else

static inline const void *machine_id_get_hashable(size_t *len)
{
	return NULL;
}

static inline void machine_id_set_hashable(const void *hashable, size_t len)
{
}

static inline int machine_id_get_app_specific(uuid_t *result, ...)
	__attribute__((__sentinel__));
static inline int machine_id_get_app_specific(uuid_t *result, ...)
{
	return -ENOSYS;
}

#endif /* CONFIG_MACHINE_ID */

#endif  /* __MACHINE_ID_H__ */
