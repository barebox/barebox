// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <of.h>

BSELFTEST_GLOBALS();

extern char __dtb_of_overlay_with_symbols_start[],
	__dtb_of_overlay_with_symbols_end[];
extern char __dtbo_of_overlay_with_symbols_start[],
	__dtbo_of_overlay_with_symbols_end[];

static struct device_node *unflatten(char *start, char *end)
{
	struct device_node *root;

	root = of_unflatten_dtb(start, end - start);
	if (IS_ERR(root)) {
		pr_err("failed to unflatten dtb");
		return NULL;
	}

	return root;
}

static bool test_setup(struct device_node **root, struct device_node **overlay,
		       char *dtb_start, char *dtb_end, char *dtbo_start,
		       char *dtbo_end)
{
	*root = unflatten(dtb_start, dtb_end);
	if (!assert_cond(*root != NULL))
		return false;

	*overlay = unflatten(dtbo_start, dtbo_end);
	if (!assert_cond(*overlay != NULL)) {
		of_delete_node(*root);
		return false;
	}

	return true;
}

static void test_of_overlay_symbol_without_overlay(void)
{
	struct device_node *root, *overlay, *symbols;
	const char *path;
	int ret;

	if (!test_setup(&root, &overlay, __dtb_of_overlay_with_symbols_start,
			__dtb_of_overlay_with_symbols_end,
			__dtbo_of_overlay_with_symbols_start,
			__dtbo_of_overlay_with_symbols_end))
		return;

	ret = of_overlay_apply_tree(root, overlay);
	assert_inteq(ret, 0);

	symbols = of_get_child_by_name(root, "__symbols__");
	assert_cond(symbols != NULL);

	assert_inteq(of_property_read_string(symbols, "added", &path), 0);
	assert_inteq(of_property_read_string(symbols, "toplevel", &path),
		     -EINVAL);
	assert_inteq(of_property_read_string(symbols, "target", &path), 0);

	of_delete_node(overlay);
	of_delete_node(root);
}

static void test_of_overlay_symbol_outside_overlay(void)
{
	struct device_node *root, *overlay, *symbols;
	const char *path;
	int ret;

	if (!test_setup(&root, &overlay, __dtb_of_overlay_with_symbols_start,
			__dtb_of_overlay_with_symbols_end,
			__dtbo_of_overlay_with_symbols_start,
			__dtbo_of_overlay_with_symbols_end))
		return;

	ret = of_overlay_apply_tree(root, overlay);
	assert_inteq(ret, 0);

	symbols = of_get_child_by_name(root, "__symbols__");
	assert_cond(symbols != NULL);

	assert_inteq(of_property_read_string(symbols, "added", &path), 0);
	assert_inteq(of_property_read_string(symbols, "local", &path), -EINVAL);
	assert_inteq(of_property_read_string(symbols, "outside", &path), -EINVAL);

	of_delete_node(overlay);
	of_delete_node(root);
}

static void test_of_overlay(void)
{
	test_of_overlay_symbol_without_overlay();
	test_of_overlay_symbol_outside_overlay();
}
bselftest(core, test_of_overlay);
