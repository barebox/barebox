// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <malloc.h>
#include <memory.h>
#include <linux/sizes.h>
#include <linux/bitops.h>

BSELFTEST_GLOBALS();

#define get_alignment(val) \
	BIT(__builtin_constant_p(val) ?  __builtin_ffsll(val) : __ffs64(val))

static_assert(get_alignment(0x1)	!= 1);
static_assert(get_alignment(0x2)	!= 2);
static_assert(get_alignment(0x3)	!= 1);
static_assert(get_alignment(0x4)	!= 4);
static_assert(get_alignment(0x5)	!= 1);
static_assert(get_alignment(0x6)	!= 2);
static_assert(get_alignment(0x8)	!= 8);
static_assert(get_alignment(0x99)	!= 0x1);
static_assert(get_alignment(0xDEADBEE0)	!= 0x10);

static bool __expect_cond(bool cond, bool expect,
			  const char *condstr, const char *func, int line)
{
	total_tests++;
	if (cond == expect)
		return true;

	failed_tests++;
	printf("%s:%d: %s to %s\n", func, line,
	       expect ? "failed" : "unexpectedly succeeded",
	       condstr);
	return false;

}

static void *__expect(void *ptr, bool expect,
		     const char *condstr, const char *func, int line)
{
	bool ok;
	total_tests++;

	ok = __expect_cond(ptr != NULL, expect, condstr, func, line);
	if (ok && ptr) {
		unsigned alignment = get_alignment((uintptr_t)ptr);
		if (alignment < CONFIG_MALLOC_ALIGNMENT) {
			failed_tests++;
			printf("%s:%d: invalid alignment of %u in %s = %p\n", func, line,
			       alignment, condstr, ptr);
		}
	}

	return ptr;
}

#define expect_alloc_ok(ptr) \
	__expect((ptr), true, #ptr, __func__, __LINE__)

#define expect_alloc_fail(ptr) \
	__expect((ptr), false, #ptr, __func__, __LINE__)

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

	p = expect_alloc_ok(malloc(1));
	free(p);

	if (mem_malloc_size) {
		tmp = expect_alloc_fail(malloc(RELOC_HIDE(SIZE_MAX, 0)));
		free(tmp);

		if (0xf0000000 > mem_malloc_size) {
			tmp = expect_alloc_fail(malloc(RELOC_HIDE(0xf0000000, 0)));
			free(tmp);
		}
	} else {
		skipped_tests += 2;
	}

	free(realloc(NULL, 1));
	p = expect_alloc_ok(realloc(NULL, 1));

	*p = 0x42;

	tmp = expect_alloc_ok(realloc(p, 2));

	p = tmp;
	__expect_cond(*p == 0x42, true, "reread after realloc", __func__, __LINE__);

	if (mem_malloc_size) {
		tmp = expect_alloc_fail(realloc(p, mem_malloc_size));
		free(tmp);

		if (0xf0000000 > mem_malloc_size) {
			tmp = expect_alloc_fail(realloc(p, RELOC_HIDE(0xf0000000, 0)));
			free(tmp);
		}

		tmp = expect_alloc_fail(realloc(p, RELOC_HIDE(SIZE_MAX, 0)));
		free(tmp);

	} else {
		skipped_tests += 3;
	}

	free(p);

	p = expect_alloc_ok(malloc(0));
	tmp = expect_alloc_ok(malloc(0));

	__expect_cond(p != tmp, true, "allocate distinct 0-size buffers", __func__, __LINE__);

	free(p);
	free(tmp);
}
bselftest(core, test_malloc);
