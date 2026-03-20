/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTM_OVERRIDES_H
#define __BOOTM_OVERRIDES_H

struct bootm_overrides {
	const char *os_file;
	const char *oftree_file;
	const char *initrd_file;
};

struct image_data;
struct loadable;

#ifdef CONFIG_BOOT_OVERRIDE
static inline void bootm_merge_overrides(struct bootm_overrides *dst,
					 const struct bootm_overrides *src)
{
	if (src->os_file)
		dst->os_file = src->os_file;
	if (src->oftree_file)
		dst->oftree_file = src->oftree_file;
	if (src->initrd_file)
		dst->initrd_file = src->initrd_file;
}

int bootm_apply_overrides(struct image_data *data,
			  const struct bootm_overrides *overrides);
void bootm_set_pending_oftree_overlays(struct loadable *oftree);
void bootm_clear_pending_oftree_overlays(void);
#else

static inline void bootm_merge_overrides(struct bootm_overrides *dst,
					 const struct bootm_overrides *src)
{
}

static inline int bootm_apply_overrides(struct image_data *data,
					const struct bootm_overrides *overrides)
{
	return 0;
}

static inline void bootm_set_pending_oftree_overlays(struct loadable *o) {}
static inline void bootm_clear_pending_oftree_overlays(void) {}
#endif

#endif
