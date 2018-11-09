/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <rtc.h>
#include <linux/rtc.h>
#include <linux/bcd.h>

struct abracon {
	struct rtc_device	rtc;
	struct i2c_client	*client;
};

static inline struct abracon *to_abracon_priv(struct rtc_device *rtcdev)
{
	return container_of(rtcdev, struct abracon, rtc);
}

static int abracon_get_time(struct rtc_device *rtcdev, struct rtc_time *t)
{
	struct abracon *abracon = to_abracon_priv(rtcdev);
	struct i2c_client *client = abracon->client;
	u8 cp[7] = {};
	u8 reg = 8;
	struct i2c_msg msg[2] = {};
	int ret;

	msg[0].addr = client->addr;
	msg[0].buf = &reg;
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = cp;
	msg[1].len = 7;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2)
		return -EIO;

	t->tm_sec = bcd2bin(cp[0]);
	t->tm_min = bcd2bin(cp[1]);
	t->tm_hour = bcd2bin(cp[2]);
	t->tm_mday = bcd2bin(cp[3]);
	t->tm_wday = bcd2bin(cp[4]);
	t->tm_mon = bcd2bin(cp[5]);
	t->tm_year = bcd2bin(cp[6]) + 100;

	return rtc_valid_tm(t);
}

static int abracon_set_time(struct rtc_device *rtcdev, struct rtc_time *t)
{
	struct abracon *abracon = to_abracon_priv(rtcdev);
	struct i2c_client *client = abracon->client;
	u8 cp[8] = {};
	int ret;

	cp[0] = 8;
	cp[1] = bin2bcd(t->tm_sec);
	cp[2] = bin2bcd(t->tm_min);
	cp[3] = bin2bcd(t->tm_hour);
	cp[4] = bin2bcd(t->tm_mday);
	cp[5] = bin2bcd(t->tm_wday);
	cp[6] = bin2bcd(t->tm_mon);
	cp[7] = bin2bcd(t->tm_year - 100);

	ret = i2c_master_send(client, cp, 8);
	if (ret != 8)
		return -EIO;

	return 0;
}

static const struct rtc_class_ops ds13xx_rtc_ops = {
	.read_time	= abracon_get_time,
	.set_time	= abracon_set_time,
};

static int abracon_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct abracon *abracon;
	int ret;

	abracon = xzalloc(sizeof(*abracon));

	abracon->client = client;

	abracon->rtc.ops = &ds13xx_rtc_ops;
	abracon->rtc.dev = dev;

	ret = rtc_register(&abracon->rtc);

	return ret;
};

static struct platform_device_id abracon_id[] = {
	{ "ab-rtcmc-32.768khz-eoz9-s3", 0 },
	{ }
};

static struct driver_d abracon_driver = {
	.name	= "rtc-abracon",
	.probe		= abracon_probe,
	.id_table	= abracon_id,
};
device_i2c_driver(abracon_driver);
