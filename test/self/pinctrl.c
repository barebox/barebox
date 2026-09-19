// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <malloc.h>
#include <linux/limits.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinctrl.h>
#include <of.h>
#include <pinctrl.h>

#include "core.h"
#include "pinctrl-utils.h"
#include "pinmux.h"

BSELFTEST_GLOBALS();

#define TEST_PINMUX(pin, func)	(((pin) << 8) | (func))
#define TEST_PINMUX_PIN(pinmux)	((pinmux) >> 8)
#define TEST_PINMUX_FUNC(pinmux)	((pinmux) & 0xff)
#define TEST_CUSTOM_PARAM	(PIN_CONFIG_END + 1)
#define TEST_CUSTOM_BOOL_ARG	7
#define TEST_CUSTOM_ENUM_DEFAULT	5	/* no index, unlike 0 */
#define TEST_NO_FAIL		UINT_MAX
#define TEST_PINMUX_DECODE_ERROR	0xffff0000

enum test_event_type {
	TEST_EVENT_REQUEST,
	TEST_EVENT_SET_MUX,
	TEST_EVENT_PINCONF,
	TEST_EVENT_PINCONF_GROUP,
};

struct test_event {
	enum test_event_type type;
	unsigned int selector;	/* pin, or group for SET_MUX/PINCONF_GROUP */
	unsigned int param;	/* function for SET_MUX, else PIN_CONFIG_* */
	u32 arg;
};

struct test_pinctrl {
	struct pinctrl_dev pdev;
	struct test_event events[32];
	unsigned int nevents;
	unsigned int fail_param;
	unsigned int fail_request;
	unsigned int fail_func;
	unsigned int map_parse_count;
	unsigned int map_free_count;
};

/* pin 7 at index 2, so pin numbers differ from their index */
static const struct pinctrl_pin_desc test_pin_descs[] = {
	PINCTRL_PIN(0, "pin0"),
	PINCTRL_PIN(1, "pin1"),
	PINCTRL_PIN(7, "pin7"),
};

static const char * const test_groups[] = {
	"grp0",
	"pin0",		/* group 1, named like pin 0 */
};

static const char * const test_functions[] = {
	"func0",
	"func1",
};

/* out of order, so pin numbers differ from their index */
static const unsigned int test_group01_pins[] = {
	1,
	0,
};

static const unsigned int test_group7_pins[] = {
	7,
};

static const struct pinfunction test_pinfunction0 = {
	.name = "func0",
};

static const struct pinfunction test_pinfunction1 = {
	.name = "func1",
};

static const char * const test_custom_enum_values[] = {
	"zero",
	"one",
	"two",
};

static const struct pinconf_generic_params test_custom_params[] = {
	{ "test,custom-bool", TEST_CUSTOM_PARAM, TEST_CUSTOM_BOOL_ARG },
	{ "test,custom-value", TEST_CUSTOM_PARAM, 0 },
	{ "test,custom-enum", TEST_CUSTOM_PARAM, TEST_CUSTOM_ENUM_DEFAULT,
	  test_custom_enum_values, ARRAY_SIZE(test_custom_enum_values) },
};

static struct test_pinctrl *to_test_pinctrl(struct pinctrl_dev *pdev)
{
	return container_of(pdev, struct test_pinctrl, pdev);
}

static int test_record_event(struct test_pinctrl *test,
			     enum test_event_type type,
			     unsigned int selector,
			     unsigned int param, u32 arg)
{
	struct test_event *event;

	if (test->nevents == ARRAY_SIZE(test->events))
		return -ENOMEM;

	event = &test->events[test->nevents++];
	event->type = type;
	event->selector = selector;
	event->param = param;
	event->arg = arg;

	return 0;
}

static int test_pinctrl_get_groups_count(struct pinctrl_dev *pdev)
{
	return ARRAY_SIZE(test_groups);
}

static const char *test_pinctrl_get_group_name(struct pinctrl_dev *pdev,
					       unsigned int selector)
{
	if (selector >= ARRAY_SIZE(test_groups))
		return NULL;

	return test_groups[selector];
}

static int test_pinctrl_get_functions_count(struct pinctrl_dev *pdev)
{
	return ARRAY_SIZE(test_functions);
}

static const char *test_pinctrl_get_function_name(struct pinctrl_dev *pdev,
						  unsigned int selector)
{
	if (selector >= ARRAY_SIZE(test_functions))
		return NULL;

	return test_functions[selector];
}

static int test_pinctrl_request(struct pinctrl_dev *pdev, unsigned int pin)
{
	struct test_pinctrl *test = to_test_pinctrl(pdev);

	if (pin == test->fail_request)
		return -EBUSY;

	return test_record_event(test, TEST_EVENT_REQUEST, pin, 0, 0);
}

static int test_pinctrl_set_mux(struct pinctrl_dev *pdev,
				unsigned int func_selector,
				unsigned int group_selector)
{
	if (func_selector == to_test_pinctrl(pdev)->fail_func)
		return -EIO;

	return test_record_event(to_test_pinctrl(pdev), TEST_EVENT_SET_MUX,
				 group_selector, func_selector, 0);
}

/*
 * U-Boot semantics: mux the pin the cell describes and return its number,
 * the pin number doubles as group selector here
 */
static int test_pinctrl_pinmux_property_set(struct pinctrl_dev *pdev,
					    u32 pinmux_group)
{
	int ret;

	if (pinmux_group == TEST_PINMUX_DECODE_ERROR)
		return -EIO;

	ret = test_pinctrl_set_mux(pdev, TEST_PINMUX_FUNC(pinmux_group),
				   TEST_PINMUX_PIN(pinmux_group));
	if (ret)
		return ret;

	return TEST_PINMUX_PIN(pinmux_group);
}

static int test_pinctrl_pin_config_set(struct pinctrl_dev *pdev,
				       unsigned int pin,
				       unsigned long *configs,
				       unsigned int num_configs)
{
	struct test_pinctrl *test = to_test_pinctrl(pdev);
	int i, ret;

	for (i = 0; i < num_configs; i++) {
		unsigned int param = pinconf_to_config_param(configs[i]);

		if (param == test->fail_param)
			return -EIO;

		ret = test_record_event(test, TEST_EVENT_PINCONF,
					pin, param,
					pinconf_to_config_argument(configs[i]));
		if (ret)
			return ret;
	}

	return 0;
}

static int test_pinctrl_pin_config_group_set(struct pinctrl_dev *pdev,
					    unsigned int group_selector,
					    unsigned long *configs,
					    unsigned int num_configs)
{
	struct test_pinctrl *test = to_test_pinctrl(pdev);
	int i, ret;

	for (i = 0; i < num_configs; i++) {
		unsigned int param = pinconf_to_config_param(configs[i]);

		if (param == test->fail_param)
			return -EIO;

		ret = test_record_event(test, TEST_EVENT_PINCONF_GROUP,
					group_selector, param,
					pinconf_to_config_argument(configs[i]));
		if (ret)
			return ret;
	}

	return 0;
}

/* Fixed groups, pins named through struct pinctrl_desc as in Linux */
static const struct pinctrl_ops test_pinctrl_ops = {
	.get_groups_count = test_pinctrl_get_groups_count,
	.get_group_name = test_pinctrl_get_group_name,
};

static const struct pinctrl_desc test_pinctrl_desc = {
	.pins = test_pin_descs,
	.npins = ARRAY_SIZE(test_pin_descs),
	.custom_params = test_custom_params,
	.num_custom_params = ARRAY_SIZE(test_custom_params),
};

static const struct pinmux_ops test_pinmux_ops = {
	.get_functions_count = test_pinctrl_get_functions_count,
	.get_function_name = test_pinctrl_get_function_name,
	.set_mux = test_pinctrl_set_mux,
	.pinmux_property_set = test_pinctrl_pinmux_property_set,
};

static const struct pinmux_ops test_pinmux_noproperty_ops = {
	.get_functions_count = test_pinctrl_get_functions_count,
	.get_function_name = test_pinctrl_get_function_name,
	.set_mux = test_pinctrl_set_mux,
};

static const struct pinconf_ops test_pinconf_ops = {
	.pin_config_set = test_pinctrl_pin_config_set,
	.pin_config_group_set = test_pinctrl_pin_config_group_set,
};

static const struct pinconf_ops test_pinconf_pin_only_ops = {
	.pin_config_set = test_pinctrl_pin_config_set,
};

static int test_pinctrl_dt_node_to_map(struct pinctrl_dev *pdev,
				       struct device_node *np_config,
				       struct pinctrl_map **map,
				       unsigned int *num_maps)
{
	struct test_pinctrl *test = to_test_pinctrl(pdev);

	test->map_parse_count++;

	if (of_property_read_bool(np_config, "test,dummy-map")) {
		*map = xzalloc(sizeof(**map));
		(*map)[0].type = PIN_MAP_TYPE_DUMMY_STATE;
		*num_maps = 1;

		return 0;
	}

	return pinconf_generic_dt_node_to_map_all(pdev, np_config, map,
						  num_maps);
}

static void test_pinctrl_dt_free_map(struct pinctrl_dev *pdev,
				     struct pinctrl_map *map,
				     unsigned int num_maps)
{
	struct test_pinctrl *test = to_test_pinctrl(pdev);

	test->map_free_count++;
	pinconf_generic_dt_free_map(pdev, map, num_maps);
}

/* Groups and functions registered at runtime with the generic helpers */
static const struct pinctrl_ops test_pinctrl_generic_ops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
};

static const struct pinctrl_ops test_pinctrl_map_ops = {
	.dt_node_to_map = test_pinctrl_dt_node_to_map,
	.dt_free_map = test_pinctrl_dt_free_map,
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
};

static const struct pinmux_ops test_pinmux_generic_ops = {
	.request = test_pinctrl_request,
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.set_mux = test_pinctrl_set_mux,
};

static const struct pinmux_ops test_pinmux_generic_norequest_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.set_mux = test_pinctrl_set_mux,
};

static void test_pinctrl_init(struct test_pinctrl *test)
{
	memset(test, 0, sizeof(*test));
	test->pdev.desc = &test_pinctrl_desc;
	test->pdev.ops = &test_pinctrl_ops;
	test->pdev.pmxops = &test_pinmux_ops;
	test->pdev.confops = &test_pinconf_ops;
	test->fail_param = TEST_NO_FAIL;
	test->fail_request = TEST_NO_FAIL;
	test->fail_func = TEST_NO_FAIL;
}

static void test_pinctrl_free_generic(struct test_pinctrl *test)
{
	free(test->pdev.groups);
	free(test->pdev.functions);
	test->pdev.groups = NULL;
	test->pdev.num_groups = 0;
	test->pdev.functions = NULL;
	test->pdev.num_functions = 0;
}

static int test_pinctrl_init_generic(struct test_pinctrl *test,
				     const struct pinctrl_ops *ops,
				     const struct pinconf_ops *confops)
{
	int ret;

	test_pinctrl_init(test);
	test->pdev.ops = ops;
	test->pdev.pmxops = &test_pinmux_generic_ops;
	test->pdev.confops = confops;

	ret = pinctrl_generic_add_group(&test->pdev, "grp01",
					test_group01_pins,
					ARRAY_SIZE(test_group01_pins), NULL);
	if (ret < 0)
		return ret;

	ret = pinmux_generic_add_pinfunction(&test->pdev,
					     &test_pinfunction0, NULL);
	if (ret < 0)
		test_pinctrl_free_generic(test);

	return ret;
}

static struct device_node *test_node(struct device_node *root, const char *path)
{
	struct device_node *np;

	np = of_find_node_by_path_from(root, path);
	assert_cond(np);

	return np;
}

static void assert_event(struct test_pinctrl *test, unsigned int index,
			 enum test_event_type type, unsigned int selector,
			 unsigned int param, u32 arg)
{
	if (!assert_cond(index < test->nevents))
		return;

	assert_inteq(test->events[index].type, type);
	assert_inteq(test->events[index].selector, selector);
	assert_inteq(test->events[index].param, param);
	assert_inteq(test->events[index].arg, arg);
}

static void test_pinmux_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/pinmux-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 5);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 1, 1, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_PULL_UP, 1);
	assert_event(&test, 2, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_DRIVE_STRENGTH, 8);
	assert_event(&test, 3, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_INPUT_ENABLE, 1);
	assert_event(&test, 4, TEST_EVENT_PINCONF, 1,
		     TEST_CUSTOM_PARAM, TEST_CUSTOM_BOOL_ARG);

	/* pinmux cells need pinmux_property_set */
	test_pinctrl_init(&test);
	test.pdev.pmxops = &test_pinmux_noproperty_ops;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -ENOTSUPP);
	assert_inteq(test.nevents, 0);
}

static void test_multi_pinmux_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/multi-pinmux-state");
	if (!np)
		return;

	/* each cell is muxed and configured */
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 4);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 1, 1, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_PULL_UP, 1);
	assert_event(&test, 2, TEST_EVENT_SET_MUX, 7, 0, 0);
	assert_event(&test, 3, TEST_EVENT_PINCONF, 7,
		     PIN_CONFIG_BIAS_PULL_UP, 1);
}

static void test_named_pins_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/named-pin-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 4);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_LEVEL, 0);
	assert_event(&test, 2, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_DISABLE, 0);
	assert_event(&test, 3, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_LEVEL, 0);

	/* pin configs need pin_config_set */
	test_pinctrl_init(&test);
	test.pdev.confops = NULL;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -ENOTSUPP);
	assert_inteq(test.nevents, 0);
}

static void test_sparse_pins_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	np = test_node(root, "/sparse-pin-state");
	if (!np)
		return;

	/* the pin number comes from the descriptor, not the table index */
	test_pinctrl_init(&test);

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 7,
		     PIN_CONFIG_BIAS_DISABLE, 0);
}

static void test_named_pins_function_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/named-pin-function-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 2);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 1, 1, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);
}

static void test_named_groups_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/named-group-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 3);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 0, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF_GROUP, 0,
		     PIN_CONFIG_SLEW_RATE, 2);
	assert_event(&test, 2, TEST_EVENT_PINCONF_GROUP, 0,
		     TEST_CUSTOM_PARAM, 9);

	/* group configs need pin_config_group_set or get_group_pins */
	test_pinctrl_init(&test);
	test.pdev.confops = &test_pinconf_pin_only_ops;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -ENOTSUPP);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 0, 0);

	/* a set_mux() error fails the state */
	test_pinctrl_init(&test);
	test.fail_func = 0;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.nevents, 0);

	/* group configs need confops, the group is muxed before */
	test_pinctrl_init(&test);
	test.pdev.confops = NULL;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EINVAL);
	assert_inteq(test.nevents, 1);

	/* the group's own selector is configured, not 0 */
	test_pinctrl_init(&test);
	np = test_node(root, "/group1-conf-state");
	if (!np)
		return;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_PINCONF_GROUP, 1,
		     PIN_CONFIG_SLEW_RATE, 3);

	/* set_mux() gets the function selector first */
	test_pinctrl_init(&test);
	np = test_node(root, "/group-function-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 1, 0);
}

/* names that match no function or group fail the state */
static void test_bad_names(struct device_node *root)
{
	static const char * const paths[] = {
		"/bad-function-state",
		"/bad-group-mux-state",
		"/bad-group-conf-state",
	};
	struct test_pinctrl test;
	struct device_node *np;
	int i;

	for (i = 0; i < ARRAY_SIZE(paths); i++) {
		test_pinctrl_init(&test);
		np = test_node(root, paths[i]);
		if (!np)
			continue;

		assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EINVAL);
		assert_inteq(test.nevents, 0);
	}
}

static void test_string_value_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/string-value-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 2);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 0, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 0,
		     TEST_CUSTOM_PARAM, 2);

	/* the first value is index 0, not an error or the default */
	test_pinctrl_init(&test);
	np = test_node(root, "/string-value-zero-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 2);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 0,
		     TEST_CUSTOM_PARAM, 0);
}

static void test_string_value_error_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/string-value-error-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -ENOENT);
	assert_inteq(test.nevents, 0);

	/* a node the generic parser rejects fails the state */
	test_pinctrl_init(&test);
	np = test_node(root, "/map-parser-error-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -ENOENT);
	assert_inteq(test.nevents, 0);
}

static void test_generic_group_registration(void)
{
	const struct function_desc *function;
	const struct group_desc *group;
	struct test_pinctrl test;
	const unsigned int *pins;
	unsigned int npins;

	test_pinctrl_init(&test);
	test.pdev.ops = &test_pinctrl_generic_ops;
	test.pdev.pmxops = &test_pinmux_generic_ops;

	assert_inteq(pinctrl_generic_get_group_count(&test.pdev), 0);
	assert_inteq(pinctrl_generic_add_group(&test.pdev, "grp01",
					       test_group01_pins,
					       ARRAY_SIZE(test_group01_pins),
					       NULL), 0);
	/* adding a known group returns its selector, like in Linux */
	assert_inteq(pinctrl_generic_add_group(&test.pdev, "grp01",
					       test_group01_pins,
					       ARRAY_SIZE(test_group01_pins),
					       NULL), 0);
	assert_inteq(pinctrl_generic_add_group(&test.pdev, NULL,
					       test_group01_pins, 1, NULL),
		     -EINVAL);
	assert_inteq(pinctrl_generic_add_group(&test.pdev, "grp1",
					       test_group01_pins, 1, &test), 1);
	assert_inteq(pinctrl_generic_add_group(&test.pdev, "grp01",
					       test_group01_pins,
					       ARRAY_SIZE(test_group01_pins),
					       NULL), 0);
	assert_inteq(pinctrl_generic_add_group(&test.pdev, "grp1",
					       test_group01_pins, 1, &test), 1);
	assert_inteq(pinctrl_generic_get_group_count(&test.pdev), 2);
	assert_streq(pinctrl_generic_get_group_name(&test.pdev, 0), "grp01");
	assert_streq(pinctrl_generic_get_group_name(&test.pdev, 1), "grp1");
	assert_cond(!pinctrl_generic_get_group_name(&test.pdev, 2));
	assert_cond(pinctrl_generic_get_group(&test.pdev, 0));
	assert_cond(!pinctrl_generic_get_group(&test.pdev, 2));
	group = pinctrl_generic_get_group(&test.pdev, 1);
	if (assert_cond(group))
		assert_cond(group->data == &test);
	assert_inteq(pinctrl_generic_get_group_pins(&test.pdev, 0, &pins,
						    &npins), 0);
	assert_cond(pins == test_group01_pins);
	assert_inteq(npins, ARRAY_SIZE(test_group01_pins));
	assert_inteq(pinctrl_get_group_selector(&test.pdev, "grp01"), 0);
	pins = NULL;
	npins = 0;
	assert_inteq(pinctrl_get_group_pins(&test.pdev, "grp01", &pins,
					    &npins), 0);
	assert_cond(pins == test_group01_pins);
	assert_inteq(npins, ARRAY_SIZE(test_group01_pins));

	assert_inteq(pinmux_generic_add_pinfunction(&test.pdev,
						    &test_pinfunction0, NULL),
		     0);
	assert_inteq(pinmux_generic_add_pinfunction(&test.pdev,
						    &test_pinfunction0, NULL),
		     0);
	assert_inteq(pinmux_generic_get_function_count(&test.pdev), 1);
	assert_inteq(pinmux_generic_add_pinfunction(&test.pdev,
						    &test_pinfunction1, &test),
		     1);
	assert_inteq(pinmux_generic_add_pinfunction(&test.pdev,
						    &test_pinfunction1, &test),
		     1);
	assert_inteq(pinmux_generic_get_function_count(&test.pdev), 2);
	assert_streq(pinmux_generic_get_function_name(&test.pdev, 0),
		     "func0");
	assert_streq(pinmux_generic_get_function_name(&test.pdev, 1),
		     "func1");
	assert_cond(!pinmux_generic_get_function_name(&test.pdev, 2));
	assert_cond(!pinmux_generic_get_function(&test.pdev, 2));
	function = pinmux_generic_get_function(&test.pdev, 1);
	if (assert_cond(function))
		assert_cond(function->data == &test);
	assert_inteq(pinmux_func_name_to_selector(&test.pdev, "func0"), 0);
	assert_inteq(pinmux_func_name_to_selector(&test.pdev, "func1"), 1);

	test_pinctrl_free_generic(&test);
}

static void test_generic_group_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	np = test_node(root, "/generic-group-state");
	if (!np)
		return;

	/* pin_config_group_set is preferred, pins are requested first */
	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_generic_ops,
						    &test_pinconf_ops), 0))
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 4);
	assert_event(&test, 0, TEST_EVENT_REQUEST, 1, 0, 0);
	assert_event(&test, 1, TEST_EVENT_REQUEST, 0, 0, 0);
	assert_event(&test, 2, TEST_EVENT_SET_MUX, 0, 0, 0);
	assert_event(&test, 3, TEST_EVENT_PINCONF_GROUP, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);

	test_pinctrl_free_generic(&test);

	/* barebox falls back to pin_config_set for each pin of the group */
	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_generic_ops,
						    &test_pinconf_pin_only_ops),
			  0))
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 5);
	assert_event(&test, 0, TEST_EVENT_REQUEST, 1, 0, 0);
	assert_event(&test, 1, TEST_EVENT_REQUEST, 0, 0, 0);
	assert_event(&test, 2, TEST_EVENT_SET_MUX, 0, 0, 0);
	assert_event(&test, 3, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_DISABLE, 0);
	assert_event(&test, 4, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);

	test_pinctrl_free_generic(&test);

	/* an error on a pin fails the state: two requests and the mux */
	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_generic_ops,
						    &test_pinconf_pin_only_ops),
			  0))
		return;

	test.fail_param = PIN_CONFIG_BIAS_DISABLE;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.nevents, 3);

	test_pinctrl_free_generic(&test);

	/* the driver can refuse a pin, the group is then not muxed */
	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_generic_ops,
						    &test_pinconf_ops), 0))
		return;

	/* grp01's second pin */
	test.fail_request = test_group01_pins[1];
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EBUSY);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_REQUEST, 1, 0, 0);

	test_pinctrl_free_generic(&test);

	/* without a request op, the pins are only muxed and configured */
	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_generic_ops,
						    &test_pinconf_ops), 0))
		return;

	test.pdev.pmxops = &test_pinmux_generic_norequest_ops;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 2);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 0, 0);

	test_pinctrl_free_generic(&test);

	/* grp01 as group 1: its own selector and pins are used, not group 0 */
	test_pinctrl_init(&test);
	test.pdev.ops = &test_pinctrl_generic_ops;
	test.pdev.pmxops = &test_pinmux_generic_ops;
	test.pdev.confops = &test_pinconf_pin_only_ops;
	assert_inteq(pinctrl_generic_add_group(&test.pdev, "grp7",
					       test_group7_pins,
					       ARRAY_SIZE(test_group7_pins),
					       NULL), 0);
	assert_inteq(pinctrl_generic_add_group(&test.pdev, "grp01",
					       test_group01_pins,
					       ARRAY_SIZE(test_group01_pins),
					       NULL), 1);
	assert_inteq(pinmux_generic_add_pinfunction(&test.pdev,
						    &test_pinfunction0, NULL),
		     0);

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 5);
	assert_event(&test, 0, TEST_EVENT_REQUEST, 1, 0, 0);
	assert_event(&test, 1, TEST_EVENT_REQUEST, 0, 0, 0);
	assert_event(&test, 2, TEST_EVENT_SET_MUX, 1, 0, 0);
	assert_event(&test, 3, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_DISABLE, 0);
	assert_event(&test, 4, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);

	test_pinctrl_free_generic(&test);

	/* the pin-by-pin fallback stops at the first pin that fails */
	np = test_node(root, "/group-error-state");
	if (!np)
		return;

	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_generic_ops,
						    &test_pinconf_pin_only_ops),
			  0))
		return;

	test.fail_param = TEST_CUSTOM_PARAM;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.nevents, 4);
	assert_event(&test, 3, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_DISABLE, 0);

	test_pinctrl_free_generic(&test);
}

static void test_dt_node_to_map(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	/* a driver's dt_node_to_map is used for the whole node like in Linux */
	np = test_node(root, "/generic-group-state");
	if (!np)
		return;

	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_map_ops,
						    &test_pinconf_ops), 0))
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.map_parse_count, 1);
	assert_inteq(test.map_free_count, 1);
	assert_inteq(test.nevents, 4);
	assert_event(&test, 0, TEST_EVENT_REQUEST, 1, 0, 0);
	assert_event(&test, 1, TEST_EVENT_REQUEST, 0, 0, 0);
	assert_event(&test, 2, TEST_EVENT_SET_MUX, 0, 0, 0);
	assert_event(&test, 3, TEST_EVENT_PINCONF_GROUP, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);

	test_pinctrl_free_generic(&test);

	/* an error applying the maps is returned, the maps are still freed */
	if (!assert_inteq(test_pinctrl_init_generic(&test,
						    &test_pinctrl_map_ops,
						    &test_pinconf_ops), 0))
		return;

	test.fail_param = PIN_CONFIG_BIAS_DISABLE;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.map_parse_count, 1);
	assert_inteq(test.map_free_count, 1);
	assert_inteq(test.nevents, 3);

	test_pinctrl_free_generic(&test);

	/* dummy maps do nothing */
	test_pinctrl_init(&test);
	test.pdev.ops = &test_pinctrl_map_ops;
	np = test_node(root, "/map-dummy-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.map_parse_count, 1);
	assert_inteq(test.map_free_count, 1);
	assert_inteq(test.nevents, 0);

	/* the core doesn't free the maps of a failed dt_node_to_map */
	test_pinctrl_init(&test);
	test.pdev.ops = &test_pinctrl_map_ops;
	np = test_node(root, "/map-parser-error-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -ENOENT);
	assert_inteq(test.map_parse_count, 1);
	assert_inteq(test.map_free_count, 0);
	assert_inteq(test.nevents, 0);

	/* the generic dt_node_to_map parses the children, the core doesn't */
	test_pinctrl_init(&test);
	test.pdev.ops = &test_pinctrl_map_ops;
	np = test_node(root, "/map-child-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.map_parse_count, 1);
	assert_inteq(test.map_free_count, 1);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);

	/* a second child's maps are appended to the first's */
	test_pinctrl_init(&test);
	test.pdev.ops = &test_pinctrl_map_ops;
	np = test_node(root, "/map-two-children-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 2);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_PULL_UP, 1);
}

static void test_child_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/child-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 2);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 1, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_LEVEL, 1);

	/* an error in a child fails the state */
	test_pinctrl_init(&test);
	test.fail_param = PIN_CONFIG_LEVEL;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.nevents, 1);

	/* disabled children are skipped, with and without dt_node_to_map */
	test_pinctrl_init(&test);
	np = test_node(root, "/disabled-child-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);

	test_pinctrl_init(&test);
	test.pdev.ops = &test_pinctrl_map_ops;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);
}

static void test_unknown_properties(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/unknown-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 0, 0);

	/* pinmux cells without configs need no confops */
	test_pinctrl_init(&test);
	test.pdev.confops = NULL;
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
}

static void test_pins_and_groups(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/pins-and-groups-state");
	if (!np)
		return;

	/* pins take precedence over groups in Linux' generic parser */
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);
}

static void test_callback_error(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	test.fail_param = TEST_CUSTOM_PARAM;

	np = test_node(root, "/error-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 0, 0, 0);

	/* the maps after the failing one are not applied */
	test_pinctrl_init(&test);
	test.fail_param = TEST_CUSTOM_PARAM;
	np = test_node(root, "/error-pins-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.nevents, 1);
	assert_event(&test, 0, TEST_EVENT_PINCONF, 0,
		     PIN_CONFIG_BIAS_DISABLE, 0);
}

static void test_pinmux_decode_error(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	np = test_node(root, "/pinmux-decode-error-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EIO);
	assert_inteq(test.nevents, 0);
}

/* barebox drivers may register in-place without a struct pinctrl_desc */
static void test_no_desc_node(struct device_node *root)
{
	struct test_pinctrl test;
	struct device_node *np;

	test_pinctrl_init(&test);
	test.pdev.desc = NULL;
	np = test_node(root, "/pinmux-state");
	if (!np)
		return;

	/* the custom parameter is the only thing that needs the descriptor */
	assert_inteq(pinctrl_map_set_state(&test.pdev, np), 0);
	assert_inteq(test.nevents, 4);
	assert_event(&test, 0, TEST_EVENT_SET_MUX, 1, 1, 0);
	assert_event(&test, 1, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_BIAS_PULL_UP, 1);
	assert_event(&test, 2, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_DRIVE_STRENGTH, 8);
	assert_event(&test, 3, TEST_EVENT_PINCONF, 1,
		     PIN_CONFIG_INPUT_ENABLE, 1);

	/* but pins can only be looked up by name through it */
	test_pinctrl_init(&test);
	test.pdev.desc = NULL;
	np = test_node(root, "/named-pin-state");
	if (!np)
		return;

	assert_inteq(pinctrl_map_set_state(&test.pdev, np), -EINVAL);
	assert_inteq(test.nevents, 0);
}

static void test_pinctrl_generic(void)
{
	extern char __dtb_pinctrl_start[], __dtb_pinctrl_end[];
	struct device_node *root;

	root = of_unflatten_dtb(__dtb_pinctrl_start,
				__dtb_pinctrl_end - __dtb_pinctrl_start);
	if (WARN_ON(IS_ERR(root)))
		return;

	test_pinmux_node(root);
	test_no_desc_node(root);
	test_multi_pinmux_node(root);
	test_named_pins_node(root);
	test_sparse_pins_node(root);
	test_named_pins_function_node(root);
	test_named_groups_node(root);
	test_bad_names(root);
	test_string_value_node(root);
	test_string_value_error_node(root);
	test_generic_group_registration();
	test_generic_group_node(root);
	test_dt_node_to_map(root);
	test_child_node(root);
	test_unknown_properties(root);
	test_pins_and_groups(root);
	test_callback_error(root);
	test_pinmux_decode_error(root);

	of_delete_node(root);
}
bselftest(core, test_pinctrl_generic);
