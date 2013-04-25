#define DEBUG

#include <common.h>
#include <errno.h>
#include <of.h>
#include <gpio.h>

int of_get_named_gpio(struct device_node *np,
                                   const char *propname, int index)
{
	int ret;
	struct device_node *gpio_np;
	const void *gpio_spec;

	ret = of_parse_phandles_with_args(np, propname, "#gpio-cells", index,
					  &gpio_np, &gpio_spec);
	if (ret) {
		pr_debug("%s: can't parse gpios property: %d\n", __func__, ret);
		return -EINVAL;
	}

	ret = gpio_get_num(gpio_np->device, be32_to_cpup(gpio_spec));
	if (ret < 0)
		return ret;

	return ret;
}
