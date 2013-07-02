#define DEBUG

#include <common.h>
#include <errno.h>
#include <of.h>
#include <gpio.h>

int of_get_named_gpio(struct device_node *np,
                                   const char *propname, int index)
{
	int ret;
	struct of_phandle_args out_args;

	ret = of_parse_phandle_with_args(np, propname, "#gpio-cells",
					index, &out_args);
	if (ret) {
		pr_debug("%s: can't parse gpios property: %d\n", __func__, ret);
		return -EINVAL;
	}

	ret = gpio_get_num(out_args.np->device, out_args.args[0]);
	if (ret < 0)
		return ret;

	return ret;
}
