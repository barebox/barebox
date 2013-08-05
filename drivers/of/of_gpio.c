#include <common.h>
#include <errno.h>
#include <of.h>
#include <of_gpio.h>
#include <gpio.h>

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
	struct of_phandle_args out_args;
	struct device_d *dev;
	int ret;

	ret = of_parse_phandle_with_args(np, propname, "#gpio-cells",
					index, &out_args);
	if (ret) {
		pr_err("%s: cannot parse %s property: %d\n",
			__func__, propname, ret);
		return ret;
	}

	dev = of_find_device_by_node(out_args.np);
	if (!dev) {
		pr_err("%s: unable to find device of node %s: %d\n",
			__func__, out_args.np->full_name, ret);
		return ret;
	}

	ret = gpio_get_num(dev, out_args.args[0]);
	if (ret < 0) {
		pr_err("%s: unable to get gpio num of device %s: %d\n",
			__func__, dev_name(dev), ret);
		return ret;
	}

	if (flags)
		*flags = out_args.args[1];

	return ret;
}
EXPORT_SYMBOL(of_get_named_gpio_flags);
