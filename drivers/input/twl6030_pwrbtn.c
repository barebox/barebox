/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <poller.h>
#include <kfifo.h>
#include <mfd/twl6030.h>
#include <twl6030_pwrbtn.h>

struct twl6030_pwrbtn_internal_data {
	int code;
	u8 previous_state;
	struct twl6030 *twl6030;
	struct kfifo *recv_fifo;
	struct console_device cdev;
	struct poller_struct poller;
};

#define PWR_PWRON_IRQ (1 << 0)

static void ic2_key_poller(struct poller_struct *poller)
{
	struct twl6030_pwrbtn_internal_data *idata = container_of(
		poller, struct twl6030_pwrbtn_internal_data, poller);
	u8 val;

	if (twl6030_reg_read(idata->twl6030, TWL6030_PMCM_HW, &val)) {
		dev_err(idata->cdev.dev, "reading i2c\n");
		return;
	}
	val = !(val & PWR_PWRON_IRQ);
	if (val != idata->previous_state && val) {
		kfifo_put(idata->recv_fifo, (u_char *)&idata->code,
			sizeof(int));
		dev_dbg(idata->cdev.dev, "pressed power button as %d\n",
			idata->code);
	}
	idata->previous_state = val;
}

static int twl6030_pwrbtn_tstc(struct console_device *cdev)
{
	struct twl6030_pwrbtn_internal_data *idata = container_of(
		cdev, struct twl6030_pwrbtn_internal_data, cdev);

	return kfifo_len(idata->recv_fifo) ? 1 : 0;
}

static int twl6030_pwrbtn_getc(struct console_device *cdev)
{
	int code = 0;
	struct twl6030_pwrbtn_internal_data *idata = container_of(
		cdev, struct twl6030_pwrbtn_internal_data, cdev);

	kfifo_get(idata->recv_fifo, (u_char *)&code, sizeof(int));
	return code;
}

static int __init twl6030_pwrbtn_probe(struct device_d *dev)
{
	struct twl6030_pwrbtn_internal_data *idata;
	struct twl6030_pwrbtn_platform_data *pdata;

	pdata = dev->platform_data;
	if (!pdata) {
		dev_err(dev, "missing platform_data\n");
		return -ENODEV;
	}

	idata = xzalloc(sizeof(struct twl6030_pwrbtn_internal_data));

	idata->recv_fifo = kfifo_alloc(sizeof(int));
	if (!idata->recv_fifo) {
		dev_err(dev, "out of memory allocating kfifo\n");
		free(idata);
		return -ENOMEM;
	}

	idata->code = pdata->code;
	idata->twl6030 = twl6030_get();
	idata->poller.func = ic2_key_poller;

	dev->type_data = &idata->cdev;
	idata->cdev.dev = dev;
	idata->cdev.tstc = twl6030_pwrbtn_tstc;
	idata->cdev.getc = twl6030_pwrbtn_getc;
	console_register(&idata->cdev);

	return poller_register(&idata->poller);
}

static struct driver_d twl6030_pwrbtn_driver = {
	.name	= "twl6030_pwrbtn",
	.probe	= twl6030_pwrbtn_probe,
};
device_platform_driver(twl6030_pwrbtn_driver);
