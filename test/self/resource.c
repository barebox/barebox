// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "test-resource: " fmt

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/ioport.h>
#include <bselftest.h>
#include <stdio.h>

struct test_ctx {
	int calls;
	struct {
		resource_size_t start;
		resource_size_t end;
		bool is_gap;
	} rec[64];
	int fail_on;
};

BSELFTEST_GLOBALS();

#define __expect_equal(x, y, desc, line) do {	\
	typeof(x) __x = (x);			\
	typeof(y) __y = (y);			\
	total_tests++;				\
	if ((__x) != (__y)) {			\
		failed_tests++;			\
		pr_err("%s:%d failed: %s == %llu, but %llu expected\n",	\
		       desc, line, #x, (u64)__x, (u64)__y);		\
	} \
} while (0)

#define expect_equal(x, y) __expect_equal((x), (y), testname, __LINE__)

static int test_cb(const struct resource *res, void *data)
{
	bool is_gap = region_is_gap(res);
	struct test_ctx *ctx = data;

	if (ctx->fail_on && ctx->calls + 1 == ctx->fail_on) {
		ctx->calls++;
		return -EIO;
	}

	ctx->rec[ctx->calls].start = res->start;
	ctx->rec[ctx->calls].end = res->end;
	ctx->rec[ctx->calls].is_gap = is_gap;
	ctx->calls++;

	return 0;
}

static void add_res(struct resource *resources,
		    resource_size_t start, resource_size_t end)
{
	struct resource *res = xzalloc(sizeof(*res));

	res->start = start;
	res->end = end;
	res->parent = resources;
	list_add_tail(&res->sibling, &resources->children);
}

static void free_reslist(struct resource *resource)
{
	struct resource *r, *tmp;

	list_for_each_entry_safe(r, tmp, &resource->children, sibling)
		free(r);
}

static int resource_list_walk(struct resource *parent,
			      int (*cb)(const struct resource *res, void *data),
			      void *data,
			      bool reverse, bool include_gaps)
{
	struct resource *res, _gap, *gap = include_gaps ? &_gap : NULL;
	int ret = 0;
	struct resource *(*start)(struct resource *, struct resource *);
	struct resource *(*advance)(struct resource *, struct resource *);

	if (reverse) {
		start = resource_iter_last;
		advance = resource_iter_prev;
	} else {
		start = resource_iter_first;
		advance = resource_iter_next;
	}

	for (res = start(parent, gap); res; res = advance(res, gap)) {
		ret = cb(res, data);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int test_resource_list_walk(const char *testname,
				   resource_size_t start,
				   resource_size_t end)
{
	struct resource resources = { .start = start, .end = end };
	struct test_ctx ctx;
	int ret, i, subranges = 0;

	INIT_LIST_HEAD(&resources.children);

	add_res(&resources, 100, 199);
	add_res(&resources, 300, 399);
	subranges = 3; /* 2 regions and range in-between */

	if (start < 100)
		subranges++;
	if (end > 399)
		subranges++;

	memset(&ctx, 0, sizeof(ctx));
	ret = resource_list_walk(&resources, test_cb, &ctx,
				 false, true);
	expect_equal(ret, 0);
	expect_equal(ctx.calls, subranges);

	i = 0;
	if (start < 100)
		expect_equal(ctx.rec[i++].is_gap, true);	/* [0–99] */
	expect_equal(ctx.rec[i++].is_gap, false);		/* [100–199] */
	expect_equal(ctx.rec[i++].is_gap, true);		/* [200–299] */
	expect_equal(ctx.rec[i++].is_gap, false);		/* [300–399] */
	if (end > 399)
		expect_equal(ctx.rec[i++].is_gap, true);	/* [400–500] */

	memset(&ctx, 0, sizeof(ctx));
	ret = resource_list_walk(&resources, test_cb, &ctx,
				 false, false);
	expect_equal(ctx.calls, 2);
	expect_equal(ctx.rec[0].start, 100);
	expect_equal(ctx.rec[1].start, 300);

	memset(&ctx, 0, sizeof(ctx));
	ctx.fail_on = 2;
	ret = resource_list_walk(&resources, test_cb, &ctx,
				 false, true);
	expect_equal(ret, -EIO);
	expect_equal(ctx.calls, 2);

	memset(&ctx, 0, sizeof(ctx));
	ret = resource_list_walk(&resources, test_cb, &ctx,
				 true, true);
	expect_equal(ret, 0);
	expect_equal(ctx.calls, subranges);

	i = 0;
	if (end > 399)
		expect_equal(ctx.rec[i++].is_gap, true);
	expect_equal(ctx.rec[i++].is_gap, false);
	expect_equal(ctx.rec[i++].is_gap, true);
	expect_equal(ctx.rec[i++].is_gap, false);
	if (start < 100)
		expect_equal(ctx.rec[i++].is_gap, true);

	i = 0;
	if (end > 399)
		expect_equal(ctx.rec[i++].start, 400);
	expect_equal(ctx.rec[i++].start, 300);
	expect_equal(ctx.rec[++i].start, 100);

	memset(&ctx, 0, sizeof(ctx));
	ret = resource_list_walk(&resources, test_cb, &ctx,
				 true, false);
	expect_equal(ctx.calls, 2);
	expect_equal(ctx.rec[0].start, 300);
	expect_equal(ctx.rec[1].start, 100);

	free_reslist(&resources);

	return 0;
}

static void test_resources(void)
{
	test_resource_list_walk("gap-around", 0, 499);
	test_resource_list_walk("gap-end", 100, 499);
	test_resource_list_walk("gap-start", 0, 399);
	test_resource_list_walk("gap-around-none", 100, 399);
}
bselftest(core, test_resources);
