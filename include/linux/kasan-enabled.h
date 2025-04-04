/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KASAN_ENABLED_H
#define _LINUX_KASAN_ENABLED_H

#ifdef CONFIG_KASAN
bool kasan_enabled(void);
#else /* CONFIG_KASAN */
static inline bool kasan_enabled(void)
{
	return IS_ENABLED(CONFIG_ASAN);
}
#endif /* CONFIG_KASAN */

#endif /* LINUX_KASAN_ENABLED_H */
