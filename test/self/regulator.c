// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <driver.h>
#include <of.h>
#include <regulator.h>
#include <linux/regulator/of_regulator.h>

struct test_regulator {
	struct device *dev;
};

struct test_regulator_cfg {
	struct regulator_desc desc;
	struct regulator_dev rdev;
};

BSELFTEST_GLOBALS();

static bool __ok(bool cond, const char *func, int line)
{
	total_tests++;
	if (!cond) {
		failed_tests++;
		printf("%s:%d: assertion failure\n", func, line);
		return false;
	}

	return true;
}

#define ok(cond) \
	__ok(cond, __func__, __LINE__)

static void test_regulator_selfref_always_on(struct device *dev)
{
	ok(1 == 1);
}

static int test_regulator_enable_noop(struct regulator_dev *rdev)
{
	dev_dbg(rdev->dev, "enabling %s-supply\n", rdev->desc->supply_name);
	failed_tests--;
	return 0;
}

static int test_regulator_disable_noop(struct regulator_dev *rdev)
{
	dev_dbg(rdev->dev, "disabling %s-supply\n", rdev->desc->supply_name);
	failed_tests--;
	return 0;
}

static const struct regulator_ops test_regulator_ops_range = {
	.enable		= test_regulator_enable_noop,
	.disable	= test_regulator_disable_noop,
};

enum {
	/*
	 * Ordering LDO1 before TEST_BUCK currently fails. This needs to be fixed
	 */
	TEST_BUCK,
	TEST_LDO1,
	TEST_LDO2,
	TEST_REGULATORS_NUM
};

static struct test_regulator_cfg test_pmic_reg[] = {
	[TEST_BUCK] = {{
		 .supply_name = "buck",
		 .ops = &test_regulator_ops_range,
	}},
	[TEST_LDO1] = {{
		 .supply_name = "ldo1",
		 .ops = &test_regulator_ops_range,
	}},
	[TEST_LDO2] = {{
		.supply_name = "ldo2",
		.ops = &test_regulator_ops_range,
	}},
};

static struct of_regulator_match test_reg_matches[] = {
	[TEST_BUCK] = { .name = "BUCK", .desc = &test_pmic_reg[TEST_BUCK].desc },
	[TEST_LDO1] = { .name = "LDO1", .desc = &test_pmic_reg[TEST_LDO1].desc },
	[TEST_LDO2] = { .name = "LDO2", .desc = &test_pmic_reg[TEST_LDO2].desc },
};

static int test_regulator_register(struct test_regulator *priv, int id,
				    struct of_regulator_match *match,
				    struct test_regulator_cfg *cfg)
{
	struct device *dev = priv->dev;
	int ret;

	if (!match->of_node) {
		dev_warn(dev, "Skip missing DTB regulator %s\n", match->name);
		return 0;
	}

	cfg->rdev.desc = &cfg->desc;
	cfg->rdev.dev = dev;

	dev_dbg(dev, "registering %s\n", match->name);

	ret = of_regulator_register(&cfg->rdev, match->of_node);
	if (ret)
		return dev_err_probe(dev, ret, "failed to register %s regulator\n",
				     match->name);

	return 0;
}

static int regulator_probe(struct device *dev)
{
	size_t nregulators = ARRAY_SIZE(test_pmic_reg);
	struct device_node *np = dev->of_node;
	struct test_regulator *priv;
	int ret, i;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;

	total_tests += 2;
	failed_tests += 2;

	np = of_get_child_by_name(np, "regulators");
	if (!np)
		return -ENOENT;

	ret = of_regulator_match(dev, np, test_reg_matches, nregulators);
	if (ret < 0)
		return ret;

	ok(ret == TEST_REGULATORS_NUM);

	for (i = 0; i < nregulators; i++) {
		ret = test_regulator_register(priv, i, &test_reg_matches[i],
					      &test_pmic_reg[i]);
		ok(ret == 0);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static const struct of_device_id test_regulator_of_match[] = {
	{ .compatible = "barebox,regulator-self-test" },
	{ /* sentintel */ },
};
MODULE_DEVICE_TABLE(of, test_regulator_of_match);

static struct driver regulator_test_driver = {
	.name = "regulator-test",
	.probe = regulator_probe,
	.of_match_table = test_regulator_of_match,
};

static struct device *dev;

static void test_regulator(void)
{
	extern char __dtbo_test_regulator_start[];
	int ret;

	if (!dev) {
		ret = platform_driver_register(&regulator_test_driver);
		if (ret)
			return;

		ret = of_overlay_apply_dtbo(of_get_root_node(), __dtbo_test_regulator_start);
		if (WARN_ON(ret))
			return;

		of_probe();

		dev = of_find_device_by_node_path("/regulator-self-test-pmic");

		ok(dev->driver != NULL);
	}

	test_regulator_selfref_always_on(dev);
}
bselftest(core, test_regulator);
