// SPDX-License-Identifier: GPL-2.0-only

#include <gpio.h>
#include <init.h>
#include <mach/rockchip/bbu.h>
#include <bootsource.h>
#include <globalvar.h>
#include <envfs.h>
#include <deep-probe.h>
#include <environment.h>
#include <linux/usb/gadget-multi.h>

struct ts433_match_data {
	const char *model;
};

static int ts433_probe(struct device *dev)
{
	const struct ts433_match_data *match;
	enum bootsource bootsource = bootsource_get();
	int copy_button_pressed = !gpio_get_value(14);

	match = device_get_match_data(dev);
	if (match)
		barebox_set_model(match->model);

	if (bootsource == BOOTSOURCE_USB || copy_button_pressed) {
		/*
		 * Configure the front USB socket to USB device (i.e. like the
		 * ROM when booting from USB. Add gadget support equivalent to
		 * usbgadget -b -A "kernel(kernel)c,initramfs(initramfs)c,dtb(dtb)" -a
		 */
		globalvar_add_simple("fastboot.bbu", "1");
		globalvar_add_simple("fastboot.partitions",
		     "/dev/mmc0(emmc),kernel(kernel)c,initramfs(initramfs)c,dtb(dtb)c");
		globalvar_add_simple("usbgadget.acm", "1");
		usbgadget_autostart(true);

		/*
		 * increase timeout to give the user the chance to open the
		 * console on USB. Fastboot automatically aborts the countdown.
		 */
		globalvar_add_simple("autoboot_timeout", "60");

		/*
		 * Don't use external env to get into a working state even with
		 * a borked environment on eMMC.
		 */
		autoload_external_env(false);
	}

	rockchip_bbu_mmc_register("emmc", BBU_HANDLER_FLAG_DEFAULT, "/dev/mmc0");

	defaultenv_append_directory(defaultenv_tsx33);

	return 0;
}

static const struct ts433_match_data ts433 = {
	.model = "QNAP TS-433"
};

static const struct ts433_match_data ts433eu = {
	.model = "QNAP TS-433eU"
};

static const struct of_device_id ts433_of_match[] = {
        { .compatible = "qnap,ts433", .data = &ts433 },
        { .compatible = "qnap,ts433eu", .data = &ts433eu },
        { /* Sentinel */},
};

static struct driver ts433_board_driver = {
        .name = "board-ts433",
        .probe = ts433_probe,
        .of_compatible = ts433_of_match,
};
coredevice_platform_driver(ts433_board_driver);

BAREBOX_DEEP_PROBE_ENABLE(ts433_of_match);
