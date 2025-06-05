/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2022 Google, Inc.
 * Written by Andrew Scull <ascull@google.com>
 */

#ifndef __TEST_FUZZ_H
#define __TEST_FUZZ_H

#include <linux/types.h>
#include <linux/compiler_types.h>
#include <ramdisk.h>

/**
 * struct fuzz_test - Information about a fuzz test
 *
 * @name: Name of fuzz test
 * @func: Function to call to perform fuzz test on an input
 */
struct fuzz_test {
	const char *name; /* must be first member */
	int (*func)(const uint8_t * data, size_t size);
};

extern const struct fuzz_test __barebox_fuzz_tests_start;
extern const struct fuzz_test __barebox_fuzz_tests_end;

#define for_each_fuzz_test(test) \
	for (test = &__barebox_fuzz_tests_start; \
	     test != &__barebox_fuzz_tests_end; test++)

#if IS_ENABLED(CONFIG_FUZZ) && IN_PROPER
/**
 * fuzz_test() - register a fuzz test
 *
 * The fuzz test function must return 0 as other values are reserved for future
 * use.
 *
 * @_name:	the name of the fuzz test function
 */
#define fuzz_test(_name, _func)					\
	static const struct fuzz_test _func##_entry		\
	__ll_elem(.barebox_fuzz_tests_##_func) = {	\
		.name = _name,					\
		.func = _func,					\
	}
#else
#define fuzz_test(_name, _func)					\
	static __always_unused void * _unused##_func = _func
#endif

#define fuzz_test_ramdisk(_name, _func)				\
	static int _func##_ramdisk(const u8 *data, size_t size)	\
	{							\
		static struct ramdisk *ramdisk;			\
		int ret;					\
		if (!ramdisk)					\
			ramdisk = ramdisk_init(512);		\
		if (!ramdisk)					\
			return -ENODEV;				\
		ramdisk_setup_ro(ramdisk, data, size);		\
		ret = _func(ramdisk_get_block_device(ramdisk));	\
		ramdisk_setup_ro(ramdisk, NULL, 0);		\
		return ret;					\
	}							\
	fuzz_test(_name, _func##_ramdisk)

static inline int fuzz_test_once(const struct fuzz_test *test, const u8 *data, size_t len)
{
	return test->func(data, len);
}

int call_for_each_fuzz_test(int (*fn)(const struct fuzz_test *test));

int setup_external_fuzz(const char *fuzz_name,
			int *argc, char ***argv);

#ifdef CONFIG_FUZZ
bool fuzz_external_active(void);
#else
static inline bool fuzz_external_active(void)
{
	return false;
}
#endif

#endif /* __TEST_FUZZ_H */
