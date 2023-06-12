/* SPDX-License-Identifier: GPL-2.0 */

#include <common.h>
#include <init.h>
#include <led.h>
#include <mach/linux.h>
#include <of.h>

static struct sandbox_led {
	struct led led;
	bool active;
} sandbox_led;

static inline void terminal_puts(const char *s)
{
	linux_write(1, s, strlen(s));
}

static void sandbox_led_set(struct led *led, unsigned int brightness)
{
	terminal_puts("\x1b]2;barebox ");
	while (brightness--)
		terminal_puts(".");
	terminal_puts("\a");

	sandbox_led.active = true;
}

static int sandbox_led_of_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret;

	if (sandbox_led.led.set)
		return -EBUSY;

	sandbox_led.led.name = xstrdup(np->name);
	sandbox_led.led.max_value = 5;
	sandbox_led.led.set = sandbox_led_set;

	ret = led_register(&sandbox_led.led);
	if (ret)
		return ret;

	led_of_parse_trigger(&sandbox_led.led, np);

	return 0;
}

static void sandbox_led_of_remove(struct device *dev)
{
	if (sandbox_led.active)
		sandbox_led_set(NULL, 0);
}

static struct of_device_id sandbox_led_of_ids[] = {
	{ .compatible = "barebox,sandbox-led", },
	{ }
};
MODULE_DEVICE_TABLE(of, sandbox_led_of_ids);

static struct driver sandbox_led_of_driver = {
	.name  = "sandbox-led",
	.probe = sandbox_led_of_probe,
	.remove = sandbox_led_of_remove,
	.of_compatible = sandbox_led_of_ids,
};
device_platform_driver(sandbox_led_of_driver);
