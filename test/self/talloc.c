// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "talloc: " fmt

#include <common.h>
#include <stdlib.h>
#include <bselftest.h>
#include <talloc.h>
#include <linux/sizes.h>
#include <linux/bitops.h>

BSELFTEST_GLOBALS();

static bool __selftest_check(bool cond, const char *func, int line)
{
	total_tests ++;
	if (cond)
		return true;

	pr_err("assertion failure at %s:%d\n", func, line);

	failed_tests++;
	return false;
}
#define selftest_check(cond) __selftest_check((cond), __func__, __LINE__)

#define STRESS_ALLOCS  1024
#define STRESS_ITERS   5000

static void talloc_basic_tests(void)
{
	void *root, *a, *b, *c;
	void *tmp;

	root = talloc_size(NULL, 0);
	selftest_check(root == ZERO_SIZE_PTR);

	root = talloc_new(NULL);
	if (!selftest_check(!ZERO_OR_NULL_PTR(root)))
		return;

	a = talloc_size(root, 128);
	selftest_check(a);

	selftest_check(talloc_parent(a) == root);

	b = talloc_zero_size(root, 256);
	selftest_check(b && memchr_inv(b, 0x00, 256) == NULL);

	c = talloc_strdup(root, "barebox talloc test");
	selftest_check(c);

	tmp = talloc_realloc_size(talloc_parent(a), a, 512);
	if (selftest_check(tmp))
		a = tmp;

	selftest_check(talloc_parent(a) == root);

	selftest_check(talloc_total_blocks(root) == 4);

	/* Free child and verify parent is intact */
	talloc_free(a);
	selftest_check(talloc_total_blocks(root) == 3);

	talloc_free(b);
	selftest_check(talloc_total_blocks(root) == 2);

	talloc_free(c);
	selftest_check(talloc_total_blocks(root) == 1);

	talloc_free(root);
}

static void talloc_hierarchy_tests(void)
{
	void *root = talloc_new(NULL);
	void *lvl1, *lvl2, *lvl3;

	lvl1 = talloc_new(root);
	lvl2 = talloc_new(lvl1);
	lvl3 = talloc_size(lvl2, 64);

	selftest_check(lvl1 && lvl2 && lvl3);

	selftest_check(talloc_parent(lvl3) == lvl2);

	talloc_free(lvl1); /* should free all children */
	selftest_check(talloc_total_blocks(root) == 1);

	talloc_free(root);
}

static void talloc_stress_tests(void)
{
	void *root = talloc_new(NULL);
	void *ptrs[STRESS_ALLOCS];

	memset(ptrs, 0, sizeof(ptrs));

	for (int iter = 0; iter < STRESS_ITERS; iter++) {
		int idx = prandom_u32_max(STRESS_ALLOCS);
		if (ptrs[idx]) {
			talloc_free(ptrs[idx]);
			ptrs[idx] = NULL;
		} else {
			size_t sz = prandom_u32_max(4096) + 1;
			ptrs[idx] = talloc_size(root, sz);
			if (selftest_check(ptrs[idx]) &&
			    prandom_u32_max(8) == 0) {
				/* Occasionally realloc to a different size */
				size_t new_sz = prandom_u32_max(8192) + 1;
				void *tmp = talloc_realloc_size(NULL,
								ptrs[idx], new_sz);
				if (selftest_check(tmp))
					ptrs[idx] = tmp;
			}
		}
	}

	for (int i = 0; i < STRESS_ALLOCS; i++) {
		if (ptrs[i])
			talloc_free(ptrs[i]);
	}
	talloc_free(root);
}

static void talloc_corner_cases(void)
{
	void *root = talloc_new(NULL);
	void *large;

	talloc_free(NULL);

	void *realloc_null = talloc_realloc_size(NULL, NULL, 128);
	selftest_check(realloc_null);
	talloc_free(realloc_null);

	/* Large alloc test (may fail gracefully if memory-constrained) */
	large = talloc_size(root, 1024 * 1024 * 10);
	talloc_free(large);

	talloc_free(root);
}

static char destructor_log[16];
static unsigned destructor_log_len;

static void reset_destructor_log(void)
{
	memset(destructor_log, 0, sizeof(destructor_log));
	destructor_log_len = 0;
}

static void log_destructor(void *data)
{
	if (destructor_log_len < sizeof(destructor_log) - 1)
		destructor_log[destructor_log_len++] = *(char *)data;
}

static void self_free_destructor(void *data)
{
	log_destructor(data);
	/* Must do nothing, we are being freed already */
	talloc_free(data);
}

static void *other_to_free;

static void other_free_destructor(void *data)
{
	log_destructor(data);
	talloc_free(other_to_free);
}

/* Chunk holding @c, with a destructor that is passed the chunk */
static void *talloc_char(const void *parent, char c, void (*fn)(void *))
{
	char *mem = talloc_size(parent, 1);

	if (!selftest_check(mem))
		return NULL;

	*mem = c;
	selftest_check(talloc_add_destructor(mem, fn, mem) == 0);

	return mem;
}

static void talloc_destructor_tests(void)
{
	void *root, *a, *b, *c, *d, *x, *tmp;

	selftest_check(talloc_add_destructor(NULL, log_destructor, "") == -EINVAL);

	root = talloc_new(NULL);

	/* Children are freed before their parent's destructors run */
	reset_destructor_log();
	a = talloc_char(root, 'a', log_destructor);
	b = talloc_char(a, 'b', log_destructor);
	c = talloc_char(a, 'c', log_destructor);
	d = talloc_char(c, 'd', log_destructor);
	if (!a || !b || !c || !d)
		return;

	talloc_free(a);
	selftest_check(!strcmp(destructor_log, "dcba"));
	selftest_check(talloc_total_blocks(root) == 1);

	/* Destructors run in reverse order, interleaved with frees */
	reset_destructor_log();
	a = talloc_new(root);
	selftest_check(talloc_add_destructor(a, log_destructor, "1") == 0);
	b = talloc_char(a, 'b', log_destructor);
	selftest_check(talloc_add_destructor(a, log_destructor, "2") == 0);
	selftest_check(talloc_total_blocks(a) == 2);
	talloc_free(a);
	selftest_check(!strcmp(destructor_log, "2b1"));

	/* Destructors survive realloc */
	reset_destructor_log();
	a = talloc_size(root, 1);
	selftest_check(talloc_add_destructor(a, log_destructor, "a") == 0);
	tmp = talloc_realloc_size(root, a, SZ_64K);
	if (selftest_check(tmp))
		a = tmp;
	talloc_free(a);
	selftest_check(!strcmp(destructor_log, "a"));

	/* Freeing a chunk from its own destructor does nothing */
	reset_destructor_log();
	a = talloc_char(root, 'a', self_free_destructor);
	talloc_free(a);
	selftest_check(!strcmp(destructor_log, "a"));
	selftest_check(talloc_total_blocks(root) == 1);

	/* Same for an ancestor that is being freed */
	reset_destructor_log();
	x = talloc_char(root, 'x', log_destructor);
	a = talloc_char(x, 'a', other_free_destructor);
	other_to_free = x;
	talloc_free(x);
	selftest_check(!strcmp(destructor_log, "ax"));
	selftest_check(talloc_total_blocks(root) == 1);

	/* Ancestors of the freed chunk can be freed */
	reset_destructor_log();
	x = talloc_char(root, 'x', log_destructor);
	a = talloc_char(x, 'a', log_destructor);
	b = talloc_char(a, 'b', other_free_destructor);
	other_to_free = x;
	talloc_free(a);
	selftest_check(!strcmp(destructor_log, "bxa"));
	selftest_check(talloc_total_blocks(root) == 1);

	/* So can siblings that are still to be freed */
	reset_destructor_log();
	a = talloc_char(root, 'a', log_destructor);
	other_to_free = talloc_char(a, 'b', log_destructor);
	c = talloc_char(a, 'c', other_free_destructor);
	talloc_free(a);
	selftest_check(!strcmp(destructor_log, "cba"));
	selftest_check(talloc_total_blocks(root) == 1);

	talloc_free(root);
}

static void test_talloc(void)
{
	talloc_basic_tests();
	talloc_hierarchy_tests();
	talloc_stress_tests();
	talloc_corner_cases();
	talloc_destructor_tests();
}
bselftest(core, test_talloc);
