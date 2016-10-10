#include <clock.h>
#include <init.h>
#include <of.h>
#include <gpio.h>
#include <printk.h>
#include <linux/kernel.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>

static int rn2120_init(void)
{
	/*
	 * This is the machine type that the kernel shipped by Netgear is using.
	 * It's wrong but a given fact.
	 */
	armlinux_set_architecture(MACH_TYPE_ARMADA_XP_DB);

	return 0;
}
device_initcall(rn2120_init);

struct hdpower {
	unsigned gpio_detect;
	unsigned gpio_power;
	unsigned gpio_led;
};

/*
 * It would be nice to have this abstracted in the device tree, but currently
 * this isn't the case.
 */
static struct hdpower rn2120_hdpower[] = {
	{
		/* sata 1 */
		.gpio_detect = 32,
		.gpio_power = 24,
		.gpio_led = 31,
	}, {
		/* sata 2 */
		.gpio_detect = 33,
		.gpio_power = 25,
		.gpio_led = 40,
	}, {
		/* sata 3 */
		.gpio_detect = 34,
		.gpio_power = 26,
		.gpio_led = 44,
	}, {
		/* sata 4 */
		.gpio_detect = 35,
		.gpio_power = 28,
		.gpio_led = 47,
	},
};

static int rn2120_hddetect(void)
{
	int i;

	if (!of_machine_is_compatible("netgear,readynas-2120"))
		return 0;

	for (i = 0; i < ARRAY_SIZE(rn2120_hdpower); ++i) {
		int ret;
		ret = gpio_direction_input(rn2120_hdpower[i].gpio_detect);
		if (ret) {
			pr_err("Failure to detect hd%d (%d)\n", i, ret);
			continue;
		}

		ret = gpio_get_value(rn2120_hdpower[i].gpio_detect);
		if (ret) {
			/* no disk present */
			gpio_direction_output(rn2120_hdpower[i].gpio_power, 0);
		} else {
			pr_info("Detected presence of disk #%d\n", i + 1);
			/* make a pause after powering up 2 disks */
			if (i && !(i & 1)) {
				pr_info("Delay power up\n");
				mdelay(7000);
			}

			gpio_direction_output(rn2120_hdpower[i].gpio_power, 1);
		}
	}
	return 0;
}
device_initcall(rn2120_hddetect);
