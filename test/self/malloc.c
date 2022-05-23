// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <malloc.h>
#include <memory.h>
#include <linux/sizes.h>

BSELFTEST_GLOBALS();

static void __expect(bool cond, bool expect,
		     const char *condstr, const char *func, int line)
{
	total_tests++;
	if (cond != expect) {
		failed_tests++;
		printf("%s:%d: %s to %s\n", func, line,
		       expect ? "failed" : "unexpectedly succeeded",
		       condstr);
	}
}

#define expect_alloc_ok(cond) \
	__expect((cond), true, #cond, __func__, __LINE__)

#define expect_alloc_fail(cond) \
	__expect((cond), false, #cond, __func__, __LINE__)

static void test_malloc(void)
{
	size_t mem_malloc_size = mem_malloc_end() - mem_malloc_start();
	u8 *p, *tmp;

	pr_debug("mem_malloc_size = %zu\n", mem_malloc_size);

	/* System libc when built for sandbox may have overcommit, so
	 * doing very big allocations without actual use may succeed
	 * unlike in-barebox allocators, so skip these tests in that
	 * case
	 */
	if (IS_ENABLED(CONFIG_MALLOC_LIBC)) {
		pr_info("built with host libc allocator: Skipping tests that may trigger overcommit\n");
		mem_malloc_size = 0;
	}

	expect_alloc_ok(p = malloc(1));
	free(p);

	if (mem_malloc_size) {
		expect_alloc_fail(malloc(SIZE_MAX));

		if (0xf0000000 > mem_malloc_size) {
			expect_alloc_fail((tmp = malloc(0xf0000000)));
			free(tmp);
		}
	} else {
		skipped_tests += 2;
	}

	p = realloc(NULL, 1);
	expect_alloc_ok(p = realloc(NULL, 1));

	*p = 0x42;

	expect_alloc_ok(tmp = realloc(p, 2));

	p = tmp;
	__expect(*p == 0x42, true, "reread after realloc", __func__, __LINE__);

	if (mem_malloc_size) {
		expect_alloc_fail(tmp = realloc(p, mem_malloc_size));

		if (0xf0000000 > mem_malloc_size)
			expect_alloc_fail((tmp = realloc(p, 0xf0000000)));

		expect_alloc_fail(tmp = realloc(p, SIZE_MAX));

	} else {
		skipped_tests += 3;
	}

	free(p);

	expect_alloc_ok(p = malloc(0));
	expect_alloc_ok(tmp = malloc(0));

	__expect(p != tmp, true, "allocate distinct 0-size buffers", __func__, __LINE__);
}
bselftest(core, test_malloc);
