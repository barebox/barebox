#ifndef __MACHINE_ID_H__
#define __MACHINE_ID_H__

#if IS_ENABLED(CONFIG_MACHINE_ID)

void machine_id_set_hashable(const void *hashable, size_t len);

#else

static inline void machine_id_set_hashable(const void *hashable, size_t len)
{
}

#endif /* CONFIG_MACHINE_ID */

#endif  /* __MACHINE_ID_H__ */
