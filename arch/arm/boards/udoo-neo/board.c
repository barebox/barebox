// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2014 Sascha Hauer, Pengutronix

#include <common.h>
#include <deep-probe.h>
#include <gpio.h>
#include <mach/bbu.h>
#include <mach/imx6.h>

/**
 * Detects the board model by checking the R184 and R185 resistors.
 * A mounted resistor (0Ohm) connects the GPIO to ground, so the
 * GPIO value will be 0.
 *
 * FULL     - Eth, WiFi, motion sensors, 1GB RAM         -> R184 not mounted - R185 mounted
 * EXTENDED - NO Eth, WiFi, motion sensors, 1GB RAM      -> R184 not mounted - R185 not mounted
 * BASE     - Eth, NO WiFi, NO motion sensors, 512MB RAM -> R184 mounted     - R185 mounted
 * BASE KS  - NO Eth, WiFi, NO motion sensors, 512MB RAM -> R184 mounted     - R185 not mounted
 */

enum imx6sx_udoneo_board_type {
	UDOO_NEO_BASIC = 0,
	UDOO_NEO_BASIC_KS = 1,
	UDOO_NEO_FULL = 2,
	UDOO_NEO_EXTENDED = 3,
	UDOO_NEO_UNKNOWN,
};

#define GPIO_R184 IMX_GPIO_NR(4, 13)
#define GPIO_R185 IMX_GPIO_NR(4, 0)

static enum imx6sx_udoneo_board_type imx6sx_udoneo_detect(struct device *dev)
{
	struct device_node *gpio_np = NULL;
	int r184, r185;
	int ret;

	gpio_np = of_find_node_by_name_address(NULL, "gpio@20a8000");
	if (gpio_np) {
		ret = of_device_ensure_probed(gpio_np);
		if (ret) {
			dev_warn(dev, "Can't probe GPIO node\n");
			goto detect_error;
		}
	} else {
		dev_warn(dev, "Can't get GPIO node\n");
		goto detect_error;
	}

	ret = gpio_request(GPIO_R184, "version r184");
	if (ret)
		goto detect_error;

	ret = gpio_request(GPIO_R185, "version r185");
	if (ret)
		goto detect_error;

	ret = gpio_direction_input(GPIO_R184);
	if (ret)
		goto detect_error;

	ret = gpio_direction_input(GPIO_R185);
	if (ret)
		goto detect_error;

	r184 = gpio_get_value(GPIO_R184);
	r185 = gpio_get_value(GPIO_R185);

	return r184 << 1 | r185 << 0;

detect_error:
	dev_warn(dev, "Board detection failed\n");

	return UDOO_NEO_UNKNOWN;
}

static int imx6sx_udoneo_probe(struct device *dev)
{
	enum imx6sx_udoneo_board_type type;
	const char *model;

	type = imx6sx_udoneo_detect(dev);
	switch (type) {
	case UDOO_NEO_FULL:
		model = "UDOO Neo Full";
		break;
	case UDOO_NEO_EXTENDED:
		model = "UDOO Neo Extended";
		break;
	case UDOO_NEO_BASIC:
		model = "UDOO Neo Basic";
		break;
	default:
		model = "UDOO Neo unknown";
	}

	barebox_set_model(model);
	barebox_set_hostname("mx6sx-udooneo");

	imx6_bbu_internal_mmc_register_handler("emmc", "/dev/mmc1.barebox",
			BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}

static const struct of_device_id imx6sx_udoneo_of_match[] = {
	{ .compatible = "udoo,neofull" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(imx6sx_udoneo_of_match);

static struct driver imx6sx_udoneo_driver = {
	.name = "board-udoo-neo",
	.probe = imx6sx_udoneo_probe,
	.of_compatible = imx6sx_udoneo_of_match,
};
postcore_platform_driver(imx6sx_udoneo_driver);
