// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * Author: Andrey Ryabinin <a.ryabinin@samsung.com>
 */

#define pr_fmt(fmt) "kasan test: " fmt

#include <common.h>
#include <command.h>
#include <complete.h>

#include "kasan.h"

/*
 * We assign some test results to these globals to make sure the tests
 * are not eliminated as dead code.
 */

int kasan_int_result;
void *kasan_ptr_result;

/*
 * Note: test functions are marked noinline so that their names appear in
 * reports.
 */

static noinline void malloc_oob_right(void)
{
	char *ptr;
	size_t size = 123;

	pr_info("out-of-bounds to right\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	OPTIMIZER_HIDE_VAR(ptr);

	ptr[size] = 'x';

	free(ptr);
}

static noinline void malloc_oob_left(void)
{
	char *ptr;
	size_t size = 15;

	pr_info("out-of-bounds to left\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	OPTIMIZER_HIDE_VAR(ptr);

	*ptr = *(ptr - 1);
	free(ptr);
}

static noinline void malloc_oob_realloc_more(void)
{
	char *ptr1, *ptr2;
	size_t size1 = 17;
	size_t size2 = 19;

	pr_info("out-of-bounds after krealloc more\n");
	ptr1 = malloc(size1);
	ptr2 = realloc(ptr1, size2);
	if (!ptr1 || !ptr2) {
		pr_err("Allocation failed\n");
		free(ptr1);
		free(ptr2);
		return;
	}

	OPTIMIZER_HIDE_VAR(ptr2);

	ptr2[size2] = 'x';

	free(ptr2);
}

static noinline void malloc_oob_realloc_less(void)
{
	char *ptr1, *ptr2;
	size_t size1 = 17;
	size_t size2 = 15;

	pr_info("out-of-bounds after krealloc less\n");
	ptr1 = malloc(size1);
	ptr2 = realloc(ptr1, size2);
	if (!ptr1 || !ptr2) {
		pr_err("Allocation failed\n");
		free(ptr1);
		return;
	}

	OPTIMIZER_HIDE_VAR(ptr2);

	ptr2[size2] = 'x';

	free(ptr2);
}

static noinline void malloc_oob_16(void)
{
	struct {
		u64 words[2];
	} *ptr1, *ptr2;

	pr_info("malloc out-of-bounds for 16-bytes access\n");
	ptr1 = malloc(sizeof(*ptr1) - 3);
	ptr2 = malloc(sizeof(*ptr2));
	if (!ptr1 || !ptr2) {
		pr_err("Allocation failed\n");
		free(ptr1);
		free(ptr2);
		return;
	}

	OPTIMIZER_HIDE_VAR(ptr1);

	*ptr1 = *ptr2;
	free(ptr1);
	free(ptr2);
}

static noinline void malloc_oob_memset_2(void)
{
	char *ptr;
	size_t size = 8;

	pr_info("out-of-bounds in memset2\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	memset(ptr + 7, 0, 2);

	free(ptr);
}

static noinline void malloc_oob_memset_4(void)
{
	char *ptr;
	size_t size = 8;

	pr_info("out-of-bounds in memset4\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	memset(ptr + 5, 0, 4);

	free(ptr);
}


static noinline void malloc_oob_memset_8(void)
{
	char *ptr;
	size_t size = 8;

	pr_info("out-of-bounds in memset8\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	memset(ptr + 1, 0, 8);

	free(ptr);
}

static noinline void malloc_oob_memset_16(void)
{
	char *ptr;
	size_t size = 16;

	pr_info("out-of-bounds in memset16\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	memset(ptr + 1, 0, 16);

	free(ptr);
}

static noinline void malloc_oob_in_memset(void)
{
	char *ptr;
	size_t size = 666;

	pr_info("out-of-bounds in memset\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	memset(ptr, 0, size + 5);

	free(ptr);
}

static noinline void malloc_uaf(void)
{
	char *ptr;
	size_t size = 10;

	pr_info("use-after-free\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	free(ptr);
	*(ptr + 8) = 'x';
}

static noinline void malloc_uaf_memset(void)
{
	char *ptr;
	size_t size = 33;

	pr_info("use-after-free in memset\n");
	ptr = malloc(size);
	if (!ptr) {
		pr_err("Allocation failed\n");
		return;
	}

	free(ptr);
	memset(ptr, 0, size);
}

static noinline void malloc_uaf2(void)
{
	char *ptr1, *ptr2;
	size_t size = 43;

	pr_info("use-after-free after another malloc\n");
	ptr1 = malloc(size);
	if (!ptr1) {
		pr_err("Allocation failed\n");
		return;
	}

	free(ptr1);
	ptr2 = malloc(size);
	if (!ptr2) {
		pr_err("Allocation failed\n");
		return;
	}

	ptr1[40] = 'x';
	if (ptr1 == ptr2)
		pr_err("Could not detect use-after-free: ptr1 == ptr2\n");
	free(ptr2);
}

static char global_array[10];

static noinline void kasan_global_oob(void)
{
	volatile int i = 3;
	char *p = &global_array[ARRAY_SIZE(global_array) + i];

	pr_info("out-of-bounds global variable\n");
	*(volatile char *)p;
}

static noinline void kasan_stack_oob(void)
{
	char stack_array[10];
	volatile int i = 0;
	char *p = &stack_array[ARRAY_SIZE(stack_array) + i];

	pr_info("out-of-bounds on stack\n");
	*(volatile char *)p;
}

static noinline void kasan_alloca_oob_left(void)
{
	volatile int i = 10;
	char alloca_array[i];
	char *p = alloca_array - 1;

	OPTIMIZER_HIDE_VAR(p);

	pr_info("out-of-bounds to left on alloca\n");
	*(volatile char *)p;
}

static noinline void kasan_alloca_oob_right(void)
{
	volatile int i = 10;
	char alloca_array[i];
	char *p = alloca_array + i;

	pr_info("out-of-bounds to right on alloca\n");
	*(volatile char *)p;
}

static noinline void kasan_memchr(void)
{
	char *ptr;
	size_t size = 24;

	pr_info("out-of-bounds in memchr\n");
	ptr = kzalloc(size, 0);
	if (!ptr)
		return;

	kasan_ptr_result = memchr(ptr, '1', size + 1);
	free(ptr);
}

static noinline void kasan_memcmp(void)
{
	char *ptr;
	size_t size = 24;
	int arr[9];

	pr_info("out-of-bounds in memcmp\n");
	ptr = kzalloc(size, 0);
	if (!ptr)
		return;

	memset(arr, 0, sizeof(arr));
	kasan_int_result = memcmp(ptr, arr, size + 1);
	free(ptr);
}

static noinline void kasan_strings(void)
{
	char *ptr;
	size_t size = 24;

	pr_info("use-after-free in strchr\n");
	ptr = malloc(size);
	if (!ptr)
		return;

	free(ptr);

	/*
	 * Try to cause only 1 invalid access (less spam in dmesg).
	 * For that we need ptr to point to zeroed byte.
	 * Skip metadata that could be stored in freed object so ptr
	 * will likely point to zeroed byte.
	 */
	ptr += 16;
	kasan_ptr_result = strchr(ptr, '1');

	pr_info("use-after-free in strrchr\n");
	kasan_ptr_result = strrchr(ptr, '1');

	pr_info("use-after-free in strcmp\n");
	kasan_int_result = strcmp(ptr, "2");

	pr_info("use-after-free in strncmp\n");
	kasan_int_result = strncmp(ptr, "2", 1);

	pr_info("use-after-free in strlen\n");
	kasan_int_result = strlen(ptr);

	pr_info("use-after-free in strnlen\n");
	kasan_int_result = strnlen(ptr, 1);
}

static noinline void kasan_bitops(void)
{
	/*
	 * Allocate 1 more byte, which causes kzalloc to round up to 16-bytes;
	 * this way we do not actually corrupt other memory.
	 */
	long *bits = xzalloc(sizeof(*bits) + 1);
	if (!bits)
		return;

	/*
	 * Below calls try to access bit within allocated memory; however, the
	 * below accesses are still out-of-bounds, since bitops are defined to
	 * operate on the whole long the bit is in.
	 */
	pr_info("out-of-bounds in set_bit\n");
	set_bit(BITS_PER_LONG, bits);

	pr_info("out-of-bounds in __set_bit\n");
	__set_bit(BITS_PER_LONG, bits);

	pr_info("out-of-bounds in clear_bit\n");
	clear_bit(BITS_PER_LONG, bits);

	pr_info("out-of-bounds in __clear_bit\n");
	__clear_bit(BITS_PER_LONG, bits);

	pr_info("out-of-bounds in change_bit\n");
	change_bit(BITS_PER_LONG, bits);

	pr_info("out-of-bounds in __change_bit\n");
	__change_bit(BITS_PER_LONG, bits);

	/*
	 * Below calls try to access bit beyond allocated memory.
	 */
	pr_info("out-of-bounds in test_and_set_bit\n");
	test_and_set_bit(BITS_PER_LONG + BITS_PER_BYTE, bits);

	pr_info("out-of-bounds in __test_and_set_bit\n");
	__test_and_set_bit(BITS_PER_LONG + BITS_PER_BYTE, bits);

	pr_info("out-of-bounds in test_and_clear_bit\n");
	test_and_clear_bit(BITS_PER_LONG + BITS_PER_BYTE, bits);

	pr_info("out-of-bounds in __test_and_clear_bit\n");
	__test_and_clear_bit(BITS_PER_LONG + BITS_PER_BYTE, bits);

	pr_info("out-of-bounds in test_and_change_bit\n");
	test_and_change_bit(BITS_PER_LONG + BITS_PER_BYTE, bits);

	pr_info("out-of-bounds in __test_and_change_bit\n");
	__test_and_change_bit(BITS_PER_LONG + BITS_PER_BYTE, bits);

	pr_info("out-of-bounds in test_bit\n");
	kasan_int_result = test_bit(BITS_PER_LONG + BITS_PER_BYTE, bits);

#if defined(clear_bit_unlock_is_negative_byte)
	pr_info("out-of-bounds in clear_bit_unlock_is_negative_byte\n");
	kasan_int_result = clear_bit_unlock_is_negative_byte(BITS_PER_LONG +
		BITS_PER_BYTE, bits);
#endif
	free(bits);
}

static int do_kasan_test(int argc, char *argv[])
{
	/*
	 * Temporarily enable multi-shot mode. Otherwise, we'd only get a
	 * report for the first case.
	 */
	bool multishot = kasan_save_enable_multi_shot();

	malloc_oob_right();
	malloc_oob_left();
	malloc_oob_realloc_more();
	malloc_oob_realloc_less();
	malloc_oob_16();
	malloc_oob_in_memset();
	malloc_oob_memset_2();
	malloc_oob_memset_4();
	malloc_oob_memset_8();
	malloc_oob_memset_16();
	malloc_uaf();
	malloc_uaf_memset();
	malloc_uaf2();
	kasan_stack_oob();
	kasan_global_oob();
	kasan_alloca_oob_left();
	kasan_alloca_oob_right();
	kasan_memchr();
	kasan_memcmp();
	kasan_strings();
	kasan_bitops();

	kasan_restore_multi_shot(multishot);

        return 0;
}

BAREBOX_CMD_START(kasan_tests)
        .cmd            = do_kasan_test,
        BAREBOX_CMD_DESC("Run KAsan tests")
        BAREBOX_CMD_GROUP(CMD_GRP_MISC)
        BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
