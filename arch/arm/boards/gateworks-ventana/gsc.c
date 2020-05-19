// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2013 Gateworks Corporation
// SPDX-FileCopyrightText: 2014 Lucas Stach, Pengutronix

/*
 * Author: Tim Harvey <tharvey@gateworks.com>
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
