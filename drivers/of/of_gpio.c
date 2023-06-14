// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <errno.h>
#include <of.h>
#include <of_gpio.h>
#include <gpio.h>

static void of_gpio_flags_quirks(struct device_node *np,
				 const char *propname,
				 enum of_gpio_flags *flags,
				 int index)
{
	/*
	 * Some GPIO fixed regulator quirks.
	 * Note that active low is the default.
	 */
	if (IS_ENABLED(CONFIG_REGULATOR) &&
	    (of_device_is_compatible(np, "regulator-fixed") ||
	     of_device_is_compatible(np, "reg-fixed-voltage") ||
	     (!(strcmp(propname, "enable-gpio") &&
		strcmp(propname, "enable-gpios")) &&
	      of_device_is_compatible(np, "regulator-gpio")))) {
		bool active_low = !of_property_read_bool(np,
							 "enable-active-high");
		/*
		 * The regulator GPIO handles are specified such that the
		 * presence or absence of "enable-active-high" solely controls
		 * the polarity of the GPIO line. Any phandle flags must
		 * be actively ignored.
		 */
		if ((*flags & OF_GPIO_ACTIVE_LOW) && !active_low) {
			pr_warn("%s GPIO handle specifies active low - ignored\n",
				np->full_name);
			*flags &= ~OF_GPIO_ACTIVE_LOW;
		}
		if (active_low)
			*flags |= OF_GPIO_ACTIVE_LOW;
	}

	/* Legacy handling of stmmac's active-low PHY reset line */
	if (IS_ENABLED(CONFIG_DRIVER_NET_DESIGNWARE_EQOS) &&
	    !strcmp(propname, "snps,reset-gpio") &&
	    of_property_read_bool(np, "snps,reset-active-low"))
		*flags |= OF_GPIO_ACTIVE_LOW;

}

/**
 * of_get_named_gpio_flags() - Get a GPIO number and flags to use with GPIO API
 * @np:		device node to get GPIO from
 * @propname:	property name containing gpio specifier(s)
 * @index:	index of the GPIO
 * @flags:	a flags pointer to fill in
 *
 * Returns GPIO number to use with GPIO API, or one of the errno value on the
 * error condition. If @flags is not NULL the function also fills in flags for
 * the GPIO.
 */
int of_get_named_gpio_flags(struct device_node *np, const char *propname,
			   int index, enum of_gpio_flags *flags)
{
	struct of_phandle_args gpiospec;
	struct device *dev;
	int ret;

	ret = of_parse_phandle_with_args(np, propname, "#gpio-cells",
					index, &gpiospec);
	if (ret) {
		pr_debug("%s: cannot parse %s property: %d\n",
			__func__, propname, ret);
		return ret;
	}

	dev = of_find_device_by_node(gpiospec.np);
	if (!dev) {
		pr_debug("%s: unable to find device of node %s\n",
			 __func__, gpiospec.np->full_name);
		ret = -EPROBE_DEFER;
		goto out;
	}

	ret = gpio_get_num(dev, gpiospec.args[0]);
	if (ret == -EPROBE_DEFER)
		goto out;
	if (ret < 0) {
		pr_err("%s: unable to get gpio num of device %s: %d\n",
			__func__, dev_name(dev), ret);
		goto out;
	}

	if (flags) {
		*flags = gpiospec.args[1];
		of_gpio_flags_quirks(np, propname, flags, index);
	}

out:
	of_node_put(gpiospec.np);

	return ret;
}
EXPORT_SYMBOL(of_get_named_gpio_flags);
