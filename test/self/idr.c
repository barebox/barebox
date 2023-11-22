// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <printk.h>
#include <linux/idr.h>
#include <bselftest.h>

BSELFTEST_GLOBALS();

#define __expect(cond, fmt, ...) ({ \
	bool __cond = (cond); \
	total_tests++; \
	\
	if (!__cond) { \
		failed_tests++; \
		printf("%s failed at %s:%d " fmt "\n", \
			#cond, __func__, __LINE__, ##__VA_ARGS__); \
	} \
	__cond; \
})

#define expect(ret, ...) __expect((ret), __VA_ARGS__)

static int cmp[3] = { 7, 1, 2};
static int sorted_cmp[3] = { 1, 2, 7};

static int test_idr_for_each(int id, void *p, void *data)
{
	expect(data == &cmp[2]);
	expect(*(int *)p == id);

	return id == 1 ? 0 : -1;
}

static int count_idr(int id, void *p, void *data)
{
	int *count = data;

	++*count;

	return 0;
}

static void test_idr(void)
{
	void *ptr;
	int id, count;

	DEFINE_IDR(idr);

	expect(idr_is_empty(&idr));

	expect(!idr_find(&idr, cmp[0]));

	id = idr_alloc_one(&idr, &cmp[0], cmp[0]);
	expect(id == cmp[0]);

	expect(!idr_is_empty(&idr));

	ptr = idr_find(&idr, cmp[0]);
	expect(ptr);
	expect(ptr == &cmp[0]);

	id = idr_alloc_one(&idr, &cmp[1], cmp[1]);
	expect(id == cmp[1]);

	id = idr_alloc_one(&idr, &cmp[2], cmp[2]);
	expect(id == cmp[2]);

	count = 0;

	idr_for_each_entry(&idr, ptr, id) {
		expect(id  ==  sorted_cmp[count]);
		expect(*(int *)ptr == sorted_cmp[count]);

		count++;

	}

	expect(count == 3);

	expect(idr_for_each(&idr, test_idr_for_each, &cmp[2]) == -1);

	count = 0;
	expect(idr_for_each(&idr, count_idr, &count) == 0);
	expect(count == 3);

	idr_remove(&idr, 1);

	count = 0;
	expect(idr_for_each(&idr, count_idr, &count) == 0);
	expect(count == 2);

	idr_remove(&idr, 7);

	count = 0;
	expect(idr_for_each(&idr, count_idr, &count) == 0);
	expect(count == 1);

	idr_remove(&idr, 2);

	count = 0;
	expect(idr_for_each(&idr, count_idr, &count) == 0);
	expect(count == 0);

	expect(idr_is_empty(&idr));

	idr_alloc_one(&idr, &cmp[0], cmp[0]);
	idr_alloc_one(&idr, &cmp[1], cmp[1]);
	idr_alloc_one(&idr, &cmp[2], cmp[2]);

	expect(!idr_is_empty(&idr));

	idr_destroy(&idr);

	expect(idr_is_empty(&idr));
}
bselftest(core, test_idr);
