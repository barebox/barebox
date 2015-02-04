/*
 * Copyright (C) 2009 Carlo Caione <carlo@carlocaione.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <fs.h>
#include <linux/stat.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <envfs.h>
#include <malloc.h>
#include <gpio.h>
#include <net.h>
#include <led.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>

#include <mach/core.h>
#include <mach/mbox.h>

struct msg_get_arm_mem {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_arm_mem get_arm_mem;
	u32 end_tag;
};

struct msg_get_clock_rate {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_clock_rate get_clock_rate;
	u32 end_tag;
};

struct msg_get_board_rev {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_board_rev get_board_rev;
	u32 end_tag;
};

struct msg_get_mac_address {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_get_mac_address get_mac_address;
	u32 end_tag;
};

static int rpi_get_arm_mem(u32 *size)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_arm_mem, msg);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_arm_mem, GET_ARM_MEMORY);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret)
		return ret;

	*size = msg->get_arm_mem.body.resp.mem_size;

	return 0;
}

static int rpi_register_clkdev(u32 clock_id, const char *name)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_clock_rate, msg);
	struct clk *clk;
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_clock_rate, GET_CLOCK_RATE);
	msg->get_clock_rate.body.req.clock_id = clock_id;

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret)
		return ret;

	clk = clk_fixed(name, msg->get_clock_rate.body.resp.rate_hz);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	if (!clk_register_clkdev(clk, NULL, name))
		return -ENODEV;

	return 0;
}

static void rpi_set_usbethaddr(void)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_get_mac_address, msg);
	int ret;

	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_mac_address, GET_MAC_ADDRESS);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret) {
		printf("bcm2835: Could not query MAC address\n");
		/* Ignore error; not critical */
		return;
	}

	eth_register_ethaddr(0, msg->get_mac_address.body.resp.mac);
}

static struct gpio_led leds[] = {
	{
		.gpio	= -EINVAL,
		.led	= {
			.name = "ACT",
		},
	}, {
		.gpio	= -EINVAL,
		.led	= {
			.name = "PWR",
		},
	},
};

static void rpi_add_led(void)
{
	int i;
	struct gpio_led *l;

	for (i = 0; i < ARRAY_SIZE(leds); i++) {
		l = &leds[i];

		if (gpio_is_valid(l->gpio))
			led_gpio_register(l);
	}

	l = &leds[0];
	if (gpio_is_valid(l->gpio))
		led_set_trigger(LED_TRIGGER_HEARTBEAT, &l->led);
}

static void rpi_b_plus_init(void)
{
	leds[0].gpio = 47;
	leds[1].gpio = 35;
	rpi_set_usbethaddr();
}

static void rpi_b_init(void)
{
	leds[0].gpio = 16;
	leds[0].active_low = 1;
	rpi_set_usbethaddr();
}

#define RPI_MODEL(_id, _name, _init)	\
	[_id] = {				\
		.name			= _name,\
		.init			= _init,\
	}
/* See comments in mbox.h for data source */
static const struct {
	const char *name;
	void (*init)(void);
} models[] = {
	RPI_MODEL(0, "Unknown model", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C0_2, "Model B (no P5)", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C0_3, "Model B (no P5)", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C1_4, "Model B", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C1_5, "Model B", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_I2C1_6, "Model B", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_A_7, "Model A", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_A_8, "Model A", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_A_9, "Model A", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_B_REV2_d, "Model B rev2", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_REV2_e, "Model B rev2", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_REV2_f, "Model B rev2", rpi_b_init),
	RPI_MODEL(BCM2835_BOARD_REV_B_PLUS, "Model B+", rpi_b_plus_init),
	RPI_MODEL(BCM2835_BOARD_REV_CM, "Compute Module", NULL),
	RPI_MODEL(BCM2835_BOARD_REV_A_PLUS, "Model A+", NULL),
};

static int rpi_board_rev = 0;

static void rpi_get_board_rev(void)
{
	int ret;
	char *name;

	BCM2835_MBOX_STACK_ALIGN(struct msg_get_board_rev, msg);
	BCM2835_MBOX_INIT_HDR(msg);
	BCM2835_MBOX_INIT_TAG(&msg->get_board_rev, GET_BOARD_REV);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg->hdr);
	if (ret) {
		printf("bcm2835: Could not query board revision\n");
		/* Ignore error; not critical */
		return;
	}

	rpi_board_rev = msg->get_board_rev.body.resp.rev;
	if (rpi_board_rev >= ARRAY_SIZE(models)) {
		printf("RPI: Board rev %u outside known range\n",
		       rpi_board_rev);
		goto unknown_rev;
	}

	if (!models[rpi_board_rev].name) {
		printf("RPI: Board rev %u unknown\n", rpi_board_rev);
		goto unknown_rev;
	}

	if (!rpi_board_rev)
		goto unknown_rev;

	name = asprintf("RaspberryPi %s (BCM2835/ARM1176JZF-S)",
			models[rpi_board_rev].name);
	barebox_set_model(name);
	free(name);

	return;

unknown_rev:
	rpi_board_rev = 0;
	barebox_set_model("RaspberryPi (BCM2835/ARM1176JZF-S)");
}

static void rpi_model_init(void)
{
	if (!models[rpi_board_rev].init)
		return;

	models[rpi_board_rev].init();
	rpi_add_led();
}

static int rpi_mem_init(void)
{
	u32 size = 0;
	int ret;

	ret = rpi_get_arm_mem(&size);
	if (ret)
		printf("could not query ARM memory size\n");

	bcm2835_add_device_sdram(size);

	return ret;
}
mem_initcall(rpi_mem_init);

static int rpi_console_init(void)
{
	rpi_get_board_rev();
	barebox_set_hostname("rpi");

	bcm2835_register_uart();
	return 0;
}
console_initcall(rpi_console_init);

static int rpi_clock_init(void)
{
	rpi_register_clkdev(BCM2835_MBOX_CLOCK_ID_EMMC, "bcm2835_mci0");
	return 0;
}
postconsole_initcall(rpi_clock_init);

static int rpi_env_init(void)
{
	struct stat s;
	const char *diskdev = "/dev/disk0.0";
	int ret;

	device_detect_by_name("mci0");

	ret = stat(diskdev, &s);
	if (ret) {
		printf("no %s. using default env\n", diskdev);
		return 0;
	}

	mkdir("/boot", 0666);
	ret = mount(diskdev, "fat", "/boot", NULL);
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		return 0;
	}

	default_environment_path_set("/boot/barebox.env");

	return 0;
}

static int rpi_devices_init(void)
{
	rpi_model_init();
	bcm2835_register_mci();
	bcm2835_register_fb();
	armlinux_set_architecture(MACH_TYPE_BCM2708);
	rpi_env_init();
	return 0;
}
late_initcall(rpi_devices_init);
