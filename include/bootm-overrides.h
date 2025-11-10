/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BOOTM_OVERRIDES_H
#define __BOOTM_OVERRIDES_H

struct bootm_overrides {
	const char *oftree_file;
	const char *initrd_file;
};

#ifdef CONFIG_BOOT_OVERRIDE
struct bootm_overrides bootm_set_overrides(const struct bootm_overrides overrides_new);
#else
static inline struct bootm_overrides bootm_set_overrides(const struct bootm_overrides overrides)
{
	return (struct bootm_overrides) {};
}
#endif

static inline void bootm_merge_overrides(struct bootm_overrides *dst,
					 const struct bootm_overrides *src)
{
	if (!IS_ENABLED(CONFIG_BOOT_OVERRIDE))
		return;
	if (src->oftree_file)
		dst->oftree_file = src->oftree_file;
	if (src->initrd_file)
		dst->initrd_file = src->initrd_file;
}

#endif
