/*
 * Copyright (C) 2013 Gateworks Corporation
 * Copyright (C) 2014 Lucas Stach, Pengutronix
 * Author: Tim Harvey <tharvey@gateworks.com>
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
 */

/*
 * The Gateworks System Controller will fail to ACK a master transaction if
 * it is busy, which can occur during its 1HZ timer tick while reading ADC's.
 * When this does occur, it will never be busy long enough to fail more than
 * 2 back-to-back transfers.  Thus we wrap i2c_read and i2c_write with
 * 3 retries.
 */

#include <common.h>
#include <i2c/i2c.h>

#include "gsc.h"

int gsc_i2c_read(struct i2c_client *client, u32 addr, u8 *buf, u16 count)
{
	int retry = 3;
	int n = 0;
	int ret;

	while (n++ < retry) {
		ret = i2c_read_reg(client, addr,  buf, count);
		if (!ret)
			break;
		pr_debug("GSC read failed\n");
		if (ret != -ENODEV)
			break;
		mdelay(10);
	}

	return ret;
}

int gsc_i2c_write(struct i2c_client *client, u32 addr, const u8 *buf, u16 count)
{
	int retry = 3;
	int n = 0;
	int ret;

	while (n++ < retry) {
		ret = i2c_write_reg(client, addr, buf, count);
		if (!ret)
			break;
		pr_debug("GSC write failed\n");
		if (ret != -ENODEV)
			break;
		mdelay(10);
	}
	mdelay(100);

	return ret;
}

char gsc_get_rev(struct i2c_client *client)
{
	int i;
	u8 model[16];

	gsc_i2c_read(client, 0x30, model, 16);
	for (i = sizeof(model) - 1; i > 0; i--) {
		if (model[i] >= 'A')
			return model[i];
	}

	return 'A';
}
