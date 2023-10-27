// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <linux/kernel.h>
#include <module.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/string.h>
#include <errno.h>
#include <of.h>

BSELFTEST_GLOBALS();

static void assert_different(struct device_node *a, struct device_node *b, int expect)
{
	int ret;

	total_tests++;

	ret = of_diff(a, b, -1);
	if (ret == expect)
		return;

	pr_warn("comparison of %s and %s failed: %u differences expected, %u found.\n",
		a->full_name, b->full_name, expect, ret);
	of_diff(a, b, 1);
	failed_tests++;
}

#define assert_equal(a, b) assert_different(a, b, 0)

static void test_of_basics(struct device_node *root)
{
	struct device_node *node1, *node2, *node21;

	node1 = of_new_node(root, "node1");
	node2 = of_new_node(root, "node2");

	assert_equal(node1, node2);

	of_property_write_bool(node2, "property1", true);

	assert_different(node1, node2, 1);

	node21 = of_new_node(node2, "node21");

	assert_different(node1, node2, 2);
	assert_equal(node1, node21);

	of_new_node(node1, "node21");

	assert_different(node1, node2, 1);

	of_property_write_bool(node1, "property1", true);

	assert_equal(node1, node2);

	of_property_write_bool(node2, "property1", false);
	of_property_write_u32(node2, "property1", 1);
	of_property_write_u32(node2, "property2", 2);

	of_property_write_u32(node1, "property3", 1);
	of_copy_property(node2, "property2", node1);
	of_rename_property(node1, "property3", "property1");

	assert_equal(node1, node2);
}

static void test_of_property_strings(struct device_node *root)
{
	struct device_node *np1, *np2, *np3, *np4;
	char properties[] = "ayy\0bee\0sea";

	np1 = of_new_node(root, "np1");
	np2 = of_new_node(root, "np2");
	np3 = of_new_node(root, "np3");
	np4 = of_new_node(root, "np4");

	of_property_sprintf(np1, "property-single", "%c%c%c", 'a', 'y', 'y');

	of_property_write_string(np2, "property-single", "ayy");

	assert_equal(np1, np2);

	of_set_property(np2, "property-multi", properties, sizeof(properties), 1);

	of_property_write_strings(np3, "property-single", "ayy", NULL);
	of_property_write_strings(np3, "property-multi",
				  "ayy", "bee", "sea", NULL);

	assert_equal(np2, np3);

	of_set_property(np1, "property-multi", properties, sizeof(properties), 1);

	assert_equal(np1, np2);

	of_set_property(np1, "property-multi", properties, sizeof(properties) - 1, 0);

	assert_different(np1, np2, 1);

	of_append_property(np4, "property-single", "ayy", 4);

	of_append_property(np4, "property-multi", "bee", 4);
	of_append_property(np4, "property-multi", "sea", 4);
	of_prepend_property(np4, "property-multi", "ayy", 4);

	assert_equal(np3, np4);
}

static void __init test_of_manipulation(void)
{
	extern char __dtb_of_manipulation_start[], __dtb_of_manipulation_end[];
	struct device_node *root = of_new_node(NULL, NULL);
	struct device_node *expected;

	test_of_basics(root);
	test_of_property_strings(root);

	expected = of_unflatten_dtb(__dtb_of_manipulation_start,
				    __dtb_of_manipulation_end - __dtb_of_manipulation_start);
	if (WARN_ON(IS_ERR(expected)))
		return;

	assert_equal(root, expected);

	of_delete_node(root);
	of_delete_node(expected);
}
bselftest(core, test_of_manipulation);
