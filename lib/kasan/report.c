// SPDX-License-Identifier: GPL-2.0
/*
 * This file contains common generic and tag-based KASAN error reporting code.
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
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <asm-generic/sections.h>

#include "kasan.h"

/* Shadow layout customization. */
#define SHADOW_BYTES_PER_BLOCK 1
#define SHADOW_BLOCKS_PER_ROW 16
#define SHADOW_BYTES_PER_ROW (SHADOW_BLOCKS_PER_ROW * SHADOW_BYTES_PER_BLOCK)
#define SHADOW_ROWS_AROUND_ADDR 2

static unsigned long kasan_flags;

#define KASAN_BIT_REPORTED	0
#define KASAN_BIT_MULTI_SHOT	1

bool kasan_save_enable_multi_shot(void)
{
	return test_and_set_bit(KASAN_BIT_MULTI_SHOT, &kasan_flags);
}
EXPORT_SYMBOL_GPL(kasan_save_enable_multi_shot);

void kasan_restore_multi_shot(bool enabled)
{
	if (!enabled)
		clear_bit(KASAN_BIT_MULTI_SHOT, &kasan_flags);
}
EXPORT_SYMBOL_GPL(kasan_restore_multi_shot);

static void print_error_description(struct kasan_access_info *info)
{
	eprintf("BUG: KASAN: %s in %pS\n",
		get_bug_type(info), (void *)info->ip);
	eprintf("%s of size %zu at addr %px\n",
		info->is_write ? "Write" : "Read", info->access_size,
		info->access_addr);
}

static void start_report(unsigned long *flags)
{
	/*
	 * Make sure we don't end up in loop.
	 */
	kasan_disable_current();
	eprintf("==================================================================\n");
}

static void end_report(unsigned long *flags)
{
	eprintf("==================================================================\n");
	kasan_enable_current();
}

static inline bool kernel_or_module_addr(const void *addr)
{
	if (addr >= (void *)_stext && addr < (void *)_end)
		return true;
	return false;
}

static void print_address_description(void *addr, u8 tag)
{
	dump_stack();
	eprintf("\n");

	if (kernel_or_module_addr(addr)) {
		eprintf("The buggy address belongs to the variable:\n");
		eprintf(" %pS\n", addr);
	}
}

static bool row_is_guilty(const void *row, const void *guilty)
{
	return (row <= guilty) && (guilty < row + SHADOW_BYTES_PER_ROW);
}

static int shadow_pointer_offset(const void *row, const void *shadow)
{
	/* The length of ">ff00ff00ff00ff00: " is
	 *    3 + (BITS_PER_LONG/8)*2 chars.
	 */
	return 3 + (BITS_PER_LONG/8)*2 + (shadow - row)*2 +
		(shadow - row) / SHADOW_BYTES_PER_BLOCK + 1;
}

static void print_shadow_for_address(const void *addr)
{
	int i;
	const void *shadow = kasan_mem_to_shadow(addr);
	const void *shadow_row;

	shadow_row = (void *)round_down((unsigned long)shadow,
					SHADOW_BYTES_PER_ROW)
		- SHADOW_ROWS_AROUND_ADDR * SHADOW_BYTES_PER_ROW;

	eprintf("Memory state around the buggy address:\n");

	for (i = -SHADOW_ROWS_AROUND_ADDR; i <= SHADOW_ROWS_AROUND_ADDR; i++) {
		const void *kaddr = kasan_shadow_to_mem(shadow_row);
		char buffer[4 + (BITS_PER_LONG/8)*2];
		char shadow_buf[SHADOW_BYTES_PER_ROW];

		snprintf(buffer, sizeof(buffer),
			(i == 0) ? ">%px: " : " %px: ", kaddr);
		/*
		 * We should not pass a shadow pointer to generic
		 * function, because generic functions may try to
		 * access kasan mapping for the passed address.
		 */
		memcpy(shadow_buf, shadow_row, SHADOW_BYTES_PER_ROW);
		print_hex_dump(KERN_ERR, buffer,
			DUMP_PREFIX_NONE, SHADOW_BYTES_PER_ROW, 1,
			shadow_buf, SHADOW_BYTES_PER_ROW, 0);

		if (row_is_guilty(shadow_row, shadow))
			eprintf("%*c\n",
				shadow_pointer_offset(shadow_row, shadow),
				'^');

		shadow_row += SHADOW_BYTES_PER_ROW;
	}
}

static bool report_enabled(void)
{
	if (kasan_depth)
		return false;
	if (test_bit(KASAN_BIT_MULTI_SHOT, &kasan_flags))
		return true;
	return !test_and_set_bit(KASAN_BIT_REPORTED, &kasan_flags);
}

static void __kasan_report(unsigned long addr, size_t size, bool is_write,
				unsigned long ip)
{
	struct kasan_access_info info;
	void *tagged_addr;
	void *untagged_addr;
	unsigned long flags;

	tagged_addr = (void *)addr;
	untagged_addr = reset_tag(tagged_addr);

	info.access_addr = tagged_addr;
	if (addr_has_shadow(untagged_addr))
		info.first_bad_addr = find_first_bad_addr(tagged_addr, size);
	else
		info.first_bad_addr = untagged_addr;
	info.access_size = size;
	info.is_write = is_write;
	info.ip = ip;

	start_report(&flags);

	print_error_description(&info);
	eprintf("\n");

	if (addr_has_shadow(untagged_addr)) {
		print_address_description(untagged_addr, get_tag(tagged_addr));
		eprintf("\n");
		print_shadow_for_address(info.first_bad_addr);
	} else {
		dump_stack();
	}

	end_report(&flags);
}

bool kasan_report(unsigned long addr, size_t size, bool is_write,
			unsigned long ip)
{
	bool ret = false;

	if (likely(report_enabled())) {
		__kasan_report(addr, size, is_write, ip);
		ret = true;
	}

	return ret;
}
