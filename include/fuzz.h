/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2022 Google, Inc.
 * Written by Andrew Scull <ascull@google.com>
 */

#ifndef __TEST_FUZZ_H
#define __TEST_FUZZ_H

#include <linux/types.h>
#include <linux/compiler_types.h>
#include <linux/bug.h>
#include <linux/string.h>
#include <ramdisk.h>

/**
 * struct fuzz_test - Information about a fuzz test
 *
 * @name: Name of fuzz test
 * @func: Function to call to perform fuzz test on an input
 * @init: Function to call once before the first input, or NULL
 */
struct fuzz_test {
	const char *name;
	int (*func)(const uint8_t * data, size_t size);
	void (*init)(void);
};

extern const struct fuzz_test __barebox_fuzz_tests_start;
extern const struct fuzz_test __barebox_fuzz_tests_end;

static inline bool fuzz_insecure_partial_digest_enabled(void)
{
	return IS_ENABLED(CONFIG_FUZZ) && IS_ENABLED(CONFIG_INSECURE) &&
	       IS_ENABLED(CONFIG_FUZZ_INSECURE_PARTIAL_DIGEST);
}

/**
 * fuzz_insecure_checksum_accepted() - accept a mismatching checksum
 *
 * Returns true if a fuzzing build should treat two differing checksums
 * as equal. Callers compare normally first and only consult this helper
 * for the mismatch case, so that non-fuzzing builds are unaffected.
 *
 * Only the lowest bit is compared: a fuzzer reaches the code behind the
 * check within a few mutations, while the rejection path stays
 * reachable for half of all inputs.
 */
static inline bool fuzz_insecure_checksum_accepted(u64 expected, u64 actual)
{
	if (!fuzz_insecure_partial_digest_enabled())
		return false;

	return ((expected ^ actual) & 1) == 0;
}

/**
 * fuzz_insecure_digest_accepted() - accept a mismatching digest
 *
 * Same as fuzz_insecure_checksum_accepted(), but for byte buffers, of
 * which only the lowest bit of the last byte is compared.
 */
static inline bool fuzz_insecure_digest_accepted(const void *expected,
						 const void *actual, size_t len)
{
	const u8 *a = expected, *b = actual;

	if (!fuzz_insecure_partial_digest_enabled() || !len)
		return false;

	return ((a[len - 1] ^ b[len - 1]) & 1) == 0;
}

#if IS_ENABLED(CONFIG_FUZZ) && IN_PROPER
#define __fuzz_test(_name, _func, _init)			\
	static const struct fuzz_test _func##_entry		\
	__ll_elem(.barebox_fuzz_tests_##_func) = {	\
		.name = _name,					\
		.func = _func,					\
		.init = _init,					\
	}
#else
#define __fuzz_test(_name, _func, _init)			\
	static __always_unused void * _unused##_func = _func
#endif

/**
 * fuzz_test() - register a fuzz test
 *
 * The fuzz test function must return 0 as other values are reserved for future
 * use.
 *
 * @_name:	the name of the fuzz test function
 * @_func:	the fuzz test function
 */
#define fuzz_test(_name, _func)	__fuzz_test(_name, _func, NULL)

/**
 * fuzz_test_init() - register a fuzz test that needs setting up
 *
 * Like fuzz_test(), but @_init is called once before the first input.
 * Anything the test needs only once belongs there, see fuzz_test_setup().
 *
 * @_name:	the name of the fuzz test function
 * @_func:	the fuzz test function
 * @_init:	the setup function
 */
#define fuzz_test_init(_name, _func, _init)	__fuzz_test(_name, _func, _init)

/*
 * Call the setup function a fuzz test passed to one of the macros below.
 * They generate a setup function of their own for the device they create,
 * and chain the test's after it.
 */
static inline void fuzz_call_init(void (*init)(void))
{
	if (init)
		init();
}

#define fuzz_test_ramdisk(_name, _func, _sector_size, _init)		\
	static struct ramdisk *_func##_ramdisk_##_sector_size##_dev;	\
	static __always_unused void					\
	_func##_ramdisk_##_sector_size##_init(void)			\
	{								\
		if (!_func##_ramdisk_##_sector_size##_dev)		\
			_func##_ramdisk_##_sector_size##_dev		\
				= ramdisk_init(_sector_size);		\
		fuzz_call_init(_init);					\
	}								\
	static int _func##_ramdisk_##_sector_size(const u8 *data,	\
						  size_t size)		\
	{							\
		struct ramdisk *ramdisk =			\
			_func##_ramdisk_##_sector_size##_dev;	\
		int ret;					\
		if (!ramdisk)					\
			return -ENODEV;				\
		ramdisk_setup_ro(ramdisk, data, size);		\
		ret = _func(ramdisk_get_block_device(ramdisk));	\
		ramdisk_setup_ro(ramdisk, NULL, 0);		\
		return ret;					\
	}							\
	__fuzz_test(_name, _func##_ramdisk_##_sector_size,		\
		    _func##_ramdisk_##_sector_size##_init)

#define fuzz_test_str(_name, _func)				\
	static int _func##_str(const u8 *_data, size_t size)	\
	{							\
		int ret;					\
		char *data = memdup_nul(_data, size);		\
		BUG_ON(!data);					\
		ret = _func(data, size);			\
		free(data);					\
		return ret;					\
	}							\
	fuzz_test(_name, _func##_str)

/**
 * fuzz_test_setup() - prepare a fuzz test before the first input
 *
 * Anything a fuzz test needs only once, like registering the device it
 * operates on, is done here and not on the first input, so an input's
 * code coverage does not depend on whether it happened to run first.
 *
 * The setup function may run more than once, e.g. once per invocation of
 * the fuzz command, so it has to cope with being called again.
 *
 * @test:	the fuzz test about to be run
 */
static inline void fuzz_test_setup(const struct fuzz_test *test)
{
	if (test->init)
		test->init();
}

static inline int fuzz_test_once(const struct fuzz_test *test, const u8 *data, size_t len)
{
	return test->func(data, len);
}

int call_for_each_fuzz_test(int (*fn)(const struct fuzz_test *test, void *), void *ctx);

void list_fuzz_tests(int (*println)(const char *));

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
