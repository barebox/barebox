// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <malloc.h>
#include <linux/err.h>
#include <linux/slab.h>

BSELFTEST_GLOBALS();

static void *freed[2];
static int frees;
static void count_free(void *mem)
{
	if (frees < ARRAY_SIZE(freed))
		freed[frees] = mem;
	frees++;
	free(mem);
}
DEFINE_FREE(count_free, void *, if (!IS_ERR_OR_NULL(_T)) count_free(_T))

static void test_cleanup(void)
{
	void *first, *second;

	/* leaving the scope must free the buffer, and only then */
	frees = 0;
	{
		void *p __free(count_free) = malloc(64);

		assert_cond(p != NULL);
		assert_cond(frees == 0);
		first = p;
	}
	assert_cond(frees == 1);
	assert_cond(freed[0] == first);
	/* the variable defined last is freed first */
	frees = 0;
	{
		void *p __free(count_free) = malloc(64);
		void *q __free(count_free) = malloc(64);

		first = p;
		second = q;
	}
	assert_cond(frees == 2);
	assert_cond(freed[0] == second);
	assert_cond(freed[1] == first);

	/* NULL must not reach the allocator */
	{
		void *p __free(free) = NULL;
		void *q __free(free_sensitive) = NULL;
		void *r __free(kfree) = NULL;
		void *s __free(kfree_sensitive) = NULL;

		assert_cond(p == NULL);
		assert_cond(q == NULL);
		assert_cond(r == NULL);
		assert_cond(s == NULL);
	}

	/* and neither may error pointers */
	{
		void *p __free(free) = ERR_PTR(-EINVAL);
		void *q __free(free_sensitive) = ERR_PTR(-EINVAL);
		void *r __free(kfree) = ERR_PTR(-ENOMEM);
		void *s __free(kfree_sensitive) = ERR_PTR(-ENOMEM);

		assert_cond(IS_ERR(p));
		assert_cond(IS_ERR(q));
		assert_cond(IS_ERR(r));
		assert_cond(IS_ERR(s));
	}

	/* no_free_ptr() inhibits the cleanup, so the buffer stays taken */
	{
		void *p __free(kfree) = kmalloc(64, GFP_KERNEL);
		assert_cond(p != NULL);
		first = no_free_ptr(p);
	}
	second = kmalloc(64, GFP_KERNEL);
	assert_cond(second != first);
	kfree(first);
	kfree(second);
}
bselftest(core, test_cleanup);
