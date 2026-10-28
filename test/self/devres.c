// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "devres: " fmt

#include <common.h>
#include <bselftest.h>
#include <dma.h>
#include <linux/bitmap.h>
#include <driver.h>
#include <linux/device.h>
#include <linux/sizes.h>

BSELFTEST_GLOBALS();

static bool __selftest_check(bool cond, const char *func, int line)
{
	total_tests++;
	if (cond)
		return true;

	pr_err("assertion failure at %s:%d\n", func, line);

	failed_tests++;
	return false;
}
#define selftest_check(cond) __selftest_check((cond), __func__, __LINE__)

static char action_log[8];
static unsigned action_log_len;

static void log_action(void *data)
{
	if (action_log_len < sizeof(action_log) - 1)
		action_log[action_log_len++] = *(char *)data;
}

static void reset_action_log(void)
{
	memset(action_log, 0, sizeof(action_log));
	action_log_len = 0;
}

static void test_devres_alloc(void)
{
	struct device *dev = device_alloc("devres-test", DEVICE_ID_DYNAMIC);
	unsigned long *bitmap;
	const char *cstr;
	char *str, *buf;
	int *arr;

	if (selftest_check(register_device(dev) == 0) == false)
		return;

	buf = devm_kmalloc(dev, 1, GFP_KERNEL);
	selftest_check(buf && PTR_IS_ALIGNED(buf, DMA_ALIGNMENT));
	buf[0] = 0xff;

	arr = devm_kzalloc_objs(dev, *arr, 100);
	selftest_check(arr && PTR_IS_ALIGNED(arr, DMA_ALIGNMENT));
	selftest_check(memchr_inv(arr, 0, 100 * sizeof(*arr)) == NULL);
	arr[0] = 42;
	arr[99] = 42;

	bitmap = devm_bitmap_zalloc(dev, 100, GFP_KERNEL);
	selftest_check(bitmap && bitmap_empty(bitmap, 100));

	str = devm_kstrdup(dev, "hello", GFP_KERNEL);
	selftest_check(str && !strcmp(str, "hello"));
	selftest_check(devm_kstrdup(dev, NULL, GFP_KERNEL) == NULL);

	cstr = devm_kstrdup_const(dev, "const", GFP_KERNEL);
	selftest_check(cstr && !strcmp(cstr, "const"));

	str = devm_kasprintf(dev, GFP_KERNEL, "%s%d", "test", 123);
	selftest_check(str && !strcmp(str, "test123"));

	selftest_check(devm_kmalloc(dev, 0, GFP_KERNEL) == ZERO_SIZE_PTR);

	/* Early frees, including the ones that must be no-ops */
	devm_kfree(dev, arr);
	devm_kfree(dev, cstr);
	devm_kfree(dev, NULL);
	devm_kfree(dev, ZERO_SIZE_PTR);

	selftest_check(buf[0] == 0xff && !strcmp(str, "test123"));
	selftest_check(dev->devres != NULL);

	unregister_device(dev);
	selftest_check(dev->devres == NULL);
	free_device(dev);
}

static void test_devres_action(void)
{
	struct device *dev = device_alloc("devres-test", DEVICE_ID_DYNAMIC);
	static char a = 'a', b = 'b', c = 'c';

	if (selftest_check(register_device(dev) == 0) == false)
		return;

	reset_action_log();

	selftest_check(devm_add_action(dev, log_action, &a) == 0);
	selftest_check(devm_add_action_or_reset(dev, log_action, &b) == 0);
	selftest_check(devm_add_action(dev, log_action, &c) == 0);
	selftest_check(action_log_len == 0);

	/* Actions run in reverse order of registration */
	unregister_device(dev);
	selftest_check(!strcmp(action_log, "cba"));
	free_device(dev);
}

static void test_devres_shutdown(void)
{
	struct device *dev = device_alloc("devres-test", DEVICE_ID_DYNAMIC);
	static char a = 'a', b = 'b';
	char *buf;

	if (selftest_check(register_device(dev) == 0) == false)
		return;

	reset_action_log();

	buf = devm_kstrdup(dev, "kept", GFP_KERNEL);
	selftest_check(devm_add_action(dev, log_action, &a) == 0);
	selftest_check(devm_add_action(dev, log_action, &b) == 0);

	/* Shutdown runs the actions, but keeps the memory */
	devres_run_actions(dev);
	selftest_check(!strcmp(action_log, "ba"));
	selftest_check(!strcmp(buf, "kept"));

	/* Releasing doesn't run them again */
	unregister_device(dev);
	selftest_check(!strcmp(action_log, "ba"));
	free_device(dev);
}

static void test_devres_region(void)
{
	/* The last page of the address space, skipped if it's in use */
	resource_size_t start = iomem_resource.end - SZ_4K + 1;
	enum system_states state = barebox_system_state;
	struct device *dev = device_alloc("devres-test", DEVICE_ID_DYNAMIC);
	struct resource *res;

	device_add_resource(dev, NULL, start, SZ_4K, IORESOURCE_MEM);
	if (selftest_check(register_device(dev) == 0) == false)
		return;

	res = devm_request_mem_resource(dev, 0);
	if (IS_ERR(res)) {
		skipped_tests++;
		goto out;
	}

	/* Shutdown keeps the region, but drops the action all the same */
	barebox_system_state = BAREBOX_EXITING;
	devres_run_actions(dev);
	barebox_system_state = state;
	if (selftest_check(IS_ERR(devm_request_mem_resource(dev, 0))))
		release_region(res);

	/* Releasing frees it */
	selftest_check(!IS_ERR(devm_request_mem_resource(dev, 0)));
	devres_release_all(dev);
	res = request_iomem_region("devres-test", start, iomem_resource.end);
	if (selftest_check(!IS_ERR(res)))
		release_region(res);
out:
	unregister_device(dev);
	free_device(dev);
}

static int probe_result;
static char *probe_buf;

static int devres_test_probe(struct device *dev)
{
	static char p = 'p';

	probe_buf = devm_kzalloc(dev, 16, GFP_KERNEL);
	strcpy(probe_buf, "probed");
	devm_add_action(dev, log_action, &p);

	return probe_result;
}

static void devres_test_remove(struct device *dev)
{
	static char r = 'r';

	/* Managed resources must still be intact during remove */
	log_action(&r);
	selftest_check(!strcmp(probe_buf, "probed"));
}

static struct driver devres_test_driver = {
	.name = "devres-test",
	.probe = devres_test_probe,
	.remove = devres_test_remove,
};

static void test_devres_probe(void)
{
	struct device *dev;

	if (selftest_check(platform_driver_register(&devres_test_driver) == 0) == false)
		return;

	/* Failed probe releases everything */
	reset_action_log();
	probe_result = -ENODEV;
	dev = device_alloc("devres-test", DEVICE_ID_DYNAMIC);
	platform_device_register(dev);
	selftest_check(dev->driver == NULL);
	selftest_check(!strcmp(action_log, "p"));
	selftest_check(dev->devres == NULL);
	unregister_device(dev);
	free_device(dev);

	/* Successful probe keeps everything until remove */
	reset_action_log();
	probe_result = 0;
	dev = device_alloc("devres-test", DEVICE_ID_DYNAMIC);
	platform_device_register(dev);
	selftest_check(dev->driver == &devres_test_driver);
	selftest_check(action_log_len == 0);
	selftest_check(dev->devres != NULL);
	unregister_device(dev);
	selftest_check(!strcmp(action_log, "rp"));
	selftest_check(dev->devres == NULL);
	free_device(dev);

	unregister_driver(&devres_test_driver);
}

static int devres_noremove_probe(struct device *dev)
{
	static char n = 'n';

	devm_add_action(dev, log_action, &n);

	return 0;
}

static struct driver devres_noremove_driver = {
	.name = "devres-noremove",
	.probe = devres_noremove_probe,
};

static void test_devres_unbind(void)
{
	struct device *dev;

	if (selftest_check(platform_driver_register(&devres_noremove_driver) == 0) == false)
		return;

	reset_action_log();
	dev = device_alloc("devres-noremove", DEVICE_ID_DYNAMIC);
	platform_device_register(dev);
	selftest_check(dev->driver == &devres_noremove_driver);
	selftest_check(dev->devres != NULL);

	/* A driver without remove callback releases its resources as well */
	unregister_driver(&devres_noremove_driver);
	selftest_check(dev->driver == NULL);
	selftest_check(dev->devres == NULL);
	selftest_check(!strcmp(action_log, "n"));

	unregister_device(dev);
	free_device(dev);
}

struct devres_parent_priv {
	struct device child;
};

static int devres_parent_probe(struct device *dev)
{
	struct devres_parent_priv *priv;

	priv = devm_kzalloc_obj(dev, *priv);
	dev_set_name(&priv->child, "devres-child");
	priv->child.id = DEVICE_ID_DYNAMIC;
	priv->child.parent = dev;

	return register_device(&priv->child);
}

static struct driver devres_parent_driver = {
	.name = "devres-parent",
	.probe = devres_parent_probe,
};

static void test_devres_child(void)
{
	struct device *dev;

	if (selftest_check(platform_driver_register(&devres_parent_driver) == 0) == false)
		return;

	dev = device_alloc("devres-parent", DEVICE_ID_DYNAMIC);
	platform_device_register(dev);
	selftest_check(dev->driver == &devres_parent_driver);
	selftest_check(!list_empty(&dev->children));

	/* The child lives in managed memory and must be unregistered first */
	unregister_device(dev);
	selftest_check(dev->devres == NULL);
	free_device(dev);

	unregister_driver(&devres_parent_driver);
}

static void test_devres(void)
{
	test_devres_alloc();
	test_devres_action();
	test_devres_shutdown();
	test_devres_region();
	test_devres_probe();
	test_devres_unbind();
	test_devres_child();
}
bselftest(core, test_devres);
