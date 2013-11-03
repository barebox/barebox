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
	barebox_set_model("RaspberryPi (BCM2835/ARM1176JZF-S)");
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
	ret = mount(diskdev, "fat", "/boot");
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		return 0;
	}

	default_environment_path = "/boot/barebox.env";

	return 0;
}

static int rpi_devices_init(void)
{
	bcm2835_register_mci();
	armlinux_set_architecture(MACH_TYPE_BCM2708);
	armlinux_set_bootparams((void *)(0x00000100));
	rpi_env_init();
	return 0;
}
late_initcall(rpi_devices_init);
