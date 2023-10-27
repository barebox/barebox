// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/rtc/rtc-pcf85363.c
 *
 * Driver for NXP PCF85363 real-time clock.
 *
 * Copyright (C) 2017 Eric Nelson
 *
 * This code is ported from linux-5.12
 * by Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <linux/regmap.h>
#include <rtc.h>
#include <linux/rtc.h>
#include <linux/bcd.h>

/*
 * Date/Time registers
 */
#define DT_100THS	0x00
#define DT_SECS		0x01
#define DT_MINUTES	0x02
#define DT_HOURS	0x03
#define DT_DAYS		0x04
#define DT_WEEKDAYS	0x05
#define DT_MONTHS	0x06
#define DT_YEARS	0x07

/*
 * control registers
 */
#define CTRL_STOP_EN	0x2e

#define STOP_EN_STOP	BIT(0)

#define RESET_CPR	0xa4

struct pcf85363 {
	struct rtc_device	rtc;
	struct regmap		*regmap;
};

static inline struct pcf85363 *to_pcf85363_priv(struct rtc_device *rtcdev)
{
	return container_of(rtcdev, struct pcf85363, rtc);
}

static int pcf85363_rtc_read_time(struct rtc_device *rtcdev,
					struct rtc_time *tm)
{
	struct device *dev = rtcdev->dev;
	struct pcf85363 *pcf85363 = to_pcf85363_priv(rtcdev);
	unsigned char buf[DT_YEARS + 1];
	int ret, len = sizeof(buf);

	/* read the RTC date and time registers all at once */
	ret = regmap_bulk_read(pcf85363->regmap, DT_100THS, buf, len);
	if (ret) {
		dev_err(dev, "%s: error %d\n", __func__, ret);
		return ret;
	}

	tm->tm_year = bcd2bin(buf[DT_YEARS]);
	/* adjust for 1900 base of rtc_time */
	tm->tm_year += 100;

	tm->tm_wday = buf[DT_WEEKDAYS] & 7;
	buf[DT_SECS] &= 0x7F;
	tm->tm_sec = bcd2bin(buf[DT_SECS]);
	buf[DT_MINUTES] &= 0x7F;
	tm->tm_min = bcd2bin(buf[DT_MINUTES]);
	tm->tm_hour = bcd2bin(buf[DT_HOURS]);
	tm->tm_mday = bcd2bin(buf[DT_DAYS]);
	tm->tm_mon = bcd2bin(buf[DT_MONTHS]) - 1;

	return 0;
}

static int pcf85363_rtc_set_time(struct rtc_device *rtcdev, struct rtc_time *tm)
{
	struct pcf85363 *pcf85363 = to_pcf85363_priv(rtcdev);
	unsigned char tmp[11];
	unsigned char *buf = &tmp[2];
	int ret;

	tmp[0] = STOP_EN_STOP;
	tmp[1] = RESET_CPR;

	buf[DT_100THS] = 0;
	buf[DT_SECS] = bin2bcd(tm->tm_sec);
	buf[DT_MINUTES] = bin2bcd(tm->tm_min);
	buf[DT_HOURS] = bin2bcd(tm->tm_hour);
	buf[DT_DAYS] = bin2bcd(tm->tm_mday);
	buf[DT_WEEKDAYS] = tm->tm_wday;
	buf[DT_MONTHS] = bin2bcd(tm->tm_mon + 1);
	buf[DT_YEARS] = bin2bcd(tm->tm_year % 100);

	ret = regmap_bulk_write(pcf85363->regmap, CTRL_STOP_EN,
				tmp, 2);
	if (ret)
		return ret;

	ret = regmap_bulk_write(pcf85363->regmap, DT_100THS,
				buf, sizeof(tmp) - 2);
	if (ret)
		return ret;

	return regmap_write(pcf85363->regmap, CTRL_STOP_EN, 0);
}

static const struct rtc_class_ops rtc_ops = {
	.read_time	= pcf85363_rtc_read_time,
	.set_time	= pcf85363_rtc_set_time,
};

static const struct regmap_config pcf85363_regmap_i2c_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x7f,
};

static int pcf85363_probe(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pcf85363 *pcf85363;
	struct regmap *regmap;

	regmap = regmap_init_i2c(client,
				&pcf85363_regmap_i2c_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	pcf85363 = xzalloc(sizeof(struct pcf85363));

	pcf85363->regmap = regmap;

	i2c_set_clientdata(client, pcf85363);

	pcf85363->rtc.ops = &rtc_ops;
	pcf85363->rtc.dev = dev;

	return rtc_register(&pcf85363->rtc);
}

static struct platform_device_id dev_ids[] = {
	{ .name = "pcf85363" },
	{ /* sentinel */ }
};

static struct driver pcf85363_driver = {
	.name		= "pcf85363",
	.probe		= pcf85363_probe,
	.id_table	= dev_ids,
};
device_i2c_driver(pcf85363_driver);

MODULE_AUTHOR("Eric Nelson");
MODULE_DESCRIPTION("pcf85363 I2C RTC driver");
MODULE_LICENSE("GPL");
