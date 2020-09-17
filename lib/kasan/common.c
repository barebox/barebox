// SPDX-License-Identifier: GPL-2.0
/*
 * This file contains common generic and tag-based KASAN code.
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * Author: Andrey Ryabinin <ryabinin.a.a@gmail.com>
 *
 * Some code borrowed from https://github.com/xairy/kasan-prototype by
 *        Andrey Konovalov <andreyknvl@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <common.h>
#include <linux/kasan.h>
#include <linux/kernel.h>

#include "kasan.h"

int kasan_depth;

void kasan_enable_current(void)
{
	kasan_depth++;
}

void kasan_disable_current(void)
{
	kasan_depth--;
}

#undef memset
void *memset(void *addr, int c, size_t len)
{
	if (!check_memory_region((unsigned long)addr, len, true, _RET_IP_))
		return NULL;

	return __memset(addr, c, len);
}

#ifdef __HAVE_ARCH_MEMMOVE
#undef memmove
void *memmove(void *dest, const void *src, size_t len)
{
	if (!check_memory_region((unsigned long)src, len, false, _RET_IP_) ||
	    !check_memory_region((unsigned long)dest, len, true, _RET_IP_))
		return NULL;

	return __memmove(dest, src, len);
}
#endif

#undef memcpy
void *memcpy(void *dest, const void *src, size_t len)
{
	if (!check_memory_region((unsigned long)src, len, false, _RET_IP_) ||
	    !check_memory_region((unsigned long)dest, len, true, _RET_IP_))
		return NULL;

	return __memcpy(dest, src, len);
}

/*
 * Poisons the shadow memory for 'size' bytes starting from 'addr'.
 * Memory addresses should be aligned to KASAN_SHADOW_SCALE_SIZE.
 */
void kasan_poison_shadow(const void *address, size_t size, u8 value)
{
	void *shadow_start, *shadow_end;

	/*
	 * Perform shadow offset calculation based on untagged address, as
	 * some of the callers (e.g. kasan_poison_object_data) pass tagged
	 * addresses to this function.
	 */
	address = reset_tag(address);

	shadow_start = kasan_mem_to_shadow(address);
	shadow_end = kasan_mem_to_shadow(address + size);

	__memset(shadow_start, value, shadow_end - shadow_start);
}

void kasan_unpoison_shadow(const void *address, size_t size)
{
	u8 tag = get_tag(address);

	/*
	 * Perform shadow offset calculation based on untagged address, as
	 * some of the callers (e.g. kasan_unpoison_object_data) pass tagged
	 * addresses to this function.
	 */
	address = reset_tag(address);

	kasan_poison_shadow(address, size, tag);

	if (size & KASAN_SHADOW_MASK) {
		u8 *shadow = (u8 *)kasan_mem_to_shadow(address + size);

		if (IS_ENABLED(CONFIG_KASAN_SW_TAGS))
			*shadow = tag;
		else
			*shadow = size & KASAN_SHADOW_MASK;
	}
}
