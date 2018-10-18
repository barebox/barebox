/*
 * rtc-ds1307.c - RTC driver for some mostly-compatible I2C chips.
 *
 *  This code was ported from linux-3.15 kernel by Antony Pavlov.
 *
 *  Copyright (C) 2005 James Chapman (ds1337 core)
 *  Copyright (C) 2006 David Brownell
 *  Copyright (C) 2009 Matthias Fuchs (rx8025 support)
 *  Copyright (C) 2012 Bertrand Achard (nvram access fixes)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <rtc.h>
#include <linux/rtc.h>
#include <linux/bcd.h>

/*
 * We can't determine type by probing, but if we expect pre-Linux code
 * to have set the chip up as a clock (turning on the oscillator and
 * setting the date and time), Linux can ignore the non-clock features.
 * That's a natural job for a factory or repair bench.
 */
enum ds_type {
	ds_1307,
	ds_1337,
	ds_1338,
	ds_1341,
	last_ds_type /* always last */
};

/* RTC registers don't differ much, except for the century flag */
#define DS1307_REG_SECS		0x00	/* 00-59 */
#	define DS1307_BIT_CH		0x80
#	define DS1340_BIT_nEOSC		0x80
#define DS1307_REG_MIN		0x01	/* 00-59 */
#define DS1307_REG_HOUR		0x02	/* 00-23, or 1-12{am,pm} */
#	define DS1307_BIT_12HR		0x40	/* in REG_HOUR */
#	define DS1307_BIT_PM		0x20	/* in REG_HOUR */
#	define DS1340_BIT_CENTURY_EN	0x80	/* in REG_HOUR */
#	define DS1340_BIT_CENTURY	0x40	/* in REG_HOUR */
#define DS1307_REG_WDAY		0x03	/* 01-07 */
#define DS1307_REG_MDAY		0x04	/* 01-31 */
#define DS1307_REG_MONTH	0x05	/* 01-12 */
#	define DS1337_BIT_CENTURY	0x80	/* in REG_MONTH */
#define DS1307_REG_YEAR		0x06	/* 00-99 */

/*
 * Other registers (control, status, alarms, trickle charge, NVRAM, etc)
 * start at 7, and they differ a LOT. Only control and status matter for
 * basic RTC date and time functionality; be careful using them.
 */
#define DS1307_REG_CONTROL	0x07		/* or ds1338, 1308 */
#	define DS1307_BIT_OUT		0x80
#	define DS1308_BIT_ECLK		0x40
#	define DS1338_BIT_OSF		0x20
#	define DS1307_BIT_SQWE		0x10
#	define DS1308_BIT_LOS		0x08
#	define DS1308_BIT_BBCLK		0x04
#	define DS1307_BIT_RS1		0x02
#	define DS1307_BIT_RS0		0x01
#define DS1337_REG_CONTROL	0x0e
#	define DS1337_BIT_nEOSC		0x80
#	define DS1339_BIT_BBSQI		0x20
#	define DS1341_BIT_EGFIL		0x20
#	define DS3231_BIT_BBSQW		0x40 /* same as BBSQI */
#	define DS1337_BIT_RS2		0x10
#	define DS1337_BIT_RS1		0x08
#	define DS1337_BIT_INTCN		0x04
#	define DS1337_BIT_A2IE		0x02
#	define DS1337_BIT_A1IE		0x01
#define DS1340_REG_CONTROL	0x07
#	define DS1340_BIT_OUT		0x80
#	define DS1340_BIT_FT		0x40
#	define DS1340_BIT_CALIB_SIGN	0x20
#	define DS1340_M_CALIBRATION	0x1f
#define DS1340_REG_FLAG		0x09
#	define DS1340_BIT_OSF		0x80
#define DS1337_REG_STATUS	0x0f
#	define DS1337_BIT_OSF		0x80
#	define DS1341_BIT_DOSF		0x40
#	define DS1341_BIT_ECLK		0x04
#	define DS1337_BIT_A2I		0x02
#	define DS1337_BIT_A1I		0x01


struct ds1307 {
	struct rtc_device	rtc;
	u8			offset; /* register's offset */
	u8			regs[11];
	enum ds_type		type;
	unsigned long		flags;
	struct i2c_client	*client;
	s32 (*read_block_data)(const struct i2c_client *client, u8 command,
			       u8 length, u8 *values);
	s32 (*write_block_data)(const struct i2c_client *client, u8 command,
				u8 length, const u8 *values);
};

static struct platform_device_id ds1307_id[] = {
	{ "ds1307", ds_1307 },
	{ "ds1337", ds_1337 },
	{ "ds1308", ds_1338 }, /* Difference 1308 to 1338 irrelevant */
	{ "ds1338", ds_1338 },
	{ "ds1341", ds_1341 },
	{ "ds3231", ds_1337 },
	{ "pt7c4338", ds_1307 },
	{ }
};

/*----------------------------------------------------------------------*/

#define BLOCK_DATA_MAX_TRIES 10

static s32 ds1307_read_block_data_once(const struct i2c_client *client,
				       u8 command, u8 length, u8 *values)
{
	s32 i, data;

	for (i = 0; i < length; i++) {
		data = i2c_smbus_read_byte_data(client, command + i);
		if (data < 0)
			return data;
		values[i] = data;
	}
	return i;
}

static s32 ds1307_read_block_data(const struct i2c_client *client, u8 command,
				  u8 length, u8 *values)
{
	u8 oldvalues[255];
	s32 ret;
	int tries = 0;

	dev_dbg(&client->dev, "ds1307_read_block_data (length=%d)\n", length);
	ret = ds1307_read_block_data_once(client, command, length, values);
	if (ret < 0)
		return ret;
	do {
		if (++tries > BLOCK_DATA_MAX_TRIES) {
			dev_err(&client->dev,
				"ds1307_read_block_data failed\n");
			return -EIO;
		}
		memcpy(oldvalues, values, length);
		ret = ds1307_read_block_data_once(client, command, length,
						  values);
		if (ret < 0)
			return ret;
	} while (memcmp(oldvalues, values, length));
	return length;
}

static s32 ds1307_write_block_data(const struct i2c_client *client, u8 command,
				   u8 length, const u8 *values)
{
	u8 currvalues[255];
	int tries = 0;

	dev_dbg(&client->dev, "ds1307_write_block_data (length=%d)\n", length);
	do {
		s32 i, ret;

		if (++tries > BLOCK_DATA_MAX_TRIES) {
			dev_err(&client->dev,
				"ds1307_write_block_data failed\n");
			return -EIO;
		}
		for (i = 0; i < length; i++) {
			ret = i2c_smbus_write_byte_data(client, command + i,
							values[i]);
			if (ret < 0)
				return ret;
		}
		ret = ds1307_read_block_data_once(client, command, length,
						  currvalues);
		if (ret < 0)
			return ret;
	} while (memcmp(currvalues, values, length));
	return length;
}

static inline struct ds1307 *to_ds1307_priv(struct rtc_device *rtcdev)
{
	return container_of(rtcdev, struct ds1307, rtc);
}

static int ds1307_get_time(struct rtc_device *rtcdev, struct rtc_time *t)
{
	struct device_d *dev = rtcdev->dev;
	struct ds1307 *ds1307 = to_ds1307_priv(rtcdev);
	int		tmp;

	/* read the RTC date and time registers all at once */
	tmp = ds1307->read_block_data(ds1307->client,
		ds1307->offset, 7, ds1307->regs);
	if (tmp != 7) {
		dev_err(dev, "%s error %d\n", "read", tmp);
		return -EIO;
	}

	dev_dbg(dev, "%s: %7ph\n", "read", ds1307->regs);

	t->tm_sec = bcd2bin(ds1307->regs[DS1307_REG_SECS] & 0x7f);
	t->tm_min = bcd2bin(ds1307->regs[DS1307_REG_MIN] & 0x7f);
	tmp = ds1307->regs[DS1307_REG_HOUR] & 0x3f;
	t->tm_hour = bcd2bin(tmp);
	t->tm_wday = bcd2bin(ds1307->regs[DS1307_REG_WDAY] & 0x07) - 1;
	t->tm_mday = bcd2bin(ds1307->regs[DS1307_REG_MDAY] & 0x3f);
	tmp = ds1307->regs[DS1307_REG_MONTH] & 0x1f;
	t->tm_mon = bcd2bin(tmp) - 1;

	/* assume 20YY not 19YY, and ignore DS1337_BIT_CENTURY */
	t->tm_year = bcd2bin(ds1307->regs[DS1307_REG_YEAR]) + 100;

	dev_dbg(dev, "%s secs=%d, mins=%d, "
		"hours=%d, mday=%d, mon=%d, year=%d, wday=%d\n",
		"read", t->tm_sec, t->tm_min,
		t->tm_hour, t->tm_mday,
		t->tm_mon, t->tm_year, t->tm_wday);

	/* initial clock setting can be undefined */
	return rtc_valid_tm(t);
}

static int ds1307_set_time(struct rtc_device *rtcdev, struct rtc_time *t)
{
	struct device_d *dev = rtcdev->dev;
	struct ds1307 *ds1307 = to_ds1307_priv(rtcdev);
	int		result;
	int		tmp;
	u8		*buf = ds1307->regs;

	dev_dbg(dev, "%s secs=%d, mins=%d, "
		"hours=%d, mday=%d, mon=%d, year=%d, wday=%d\n",
		"write", t->tm_sec, t->tm_min,
		t->tm_hour, t->tm_mday,
		t->tm_mon, t->tm_year, t->tm_wday);

	buf[DS1307_REG_SECS] = bin2bcd(t->tm_sec);
	buf[DS1307_REG_MIN] = bin2bcd(t->tm_min);
	buf[DS1307_REG_HOUR] = bin2bcd(t->tm_hour);
	buf[DS1307_REG_WDAY] = bin2bcd(t->tm_wday + 1);
	buf[DS1307_REG_MDAY] = bin2bcd(t->tm_mday);
	buf[DS1307_REG_MONTH] = bin2bcd(t->tm_mon + 1);

	/* assume 20YY not 19YY */
	tmp = t->tm_year - 100;
	buf[DS1307_REG_YEAR] = bin2bcd(tmp);

	switch (ds1307->type) {
	case ds_1337:
	case ds_1341:
		buf[DS1307_REG_MONTH] |= DS1337_BIT_CENTURY;
		break;
	default:
		break;
	}


	dev_dbg(dev, "%s: %7ph\n", "write", buf);

	result = ds1307->write_block_data(ds1307->client,
		ds1307->offset, 7, buf);
	if (result < 0) {
		dev_err(dev, "%s error %d\n", "write", result);
		return result;
	}
	return 0;
}

static const struct rtc_class_ops ds13xx_rtc_ops = {
	.read_time	= ds1307_get_time,
	.set_time	= ds1307_set_time,
};

static int ds1307_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ds1307		*ds1307;
	int			err = -ENODEV;
	int			tmp;
	unsigned char		*buf;
	unsigned long driver_data;
	const struct device_node *np = dev->device_node;

	ds1307 = xzalloc(sizeof(struct ds1307));

	err = dev_get_drvdata(dev, (const void **)&driver_data);
	if (err)
		goto exit;

	ds1307->client = client;
	ds1307->type = driver_data;

	buf = ds1307->regs;

	ds1307->read_block_data = ds1307_read_block_data;
	ds1307->write_block_data = ds1307_write_block_data;


	switch (ds1307->type) {
	case ds_1337:
	case ds_1341:
		/* get registers that the "rtc" read below won't read... */
		tmp = ds1307->read_block_data(ds1307->client,
					      DS1337_REG_CONTROL, 2, buf);

		if (tmp != 2) {
			dev_dbg(&client->dev, "read error %d\n", tmp);
			err = -EIO;
			goto exit;
		}

		/* oscillator off?  turn it on, so clock can tick. */
		if (ds1307->regs[0] & DS1337_BIT_nEOSC)
			ds1307->regs[0] &= ~DS1337_BIT_nEOSC;


		/*
		  Make sure no alarm interrupts or square wave signals
		  are produced by the chip while we are in
		  bootloader. We do this by configuring the RTC to
		  generate alarm interrupts (thus disabling square
		  wave generation), but disabling each individual
		  alarm interrupt source
		 */
		ds1307->regs[0] |= DS1337_BIT_INTCN;
		ds1307->regs[0] &= ~(DS1337_BIT_A2IE | DS1337_BIT_A1IE);

		i2c_smbus_write_byte_data(client, DS1337_REG_CONTROL,
					  ds1307->regs[0]);

		if (ds1307->type == ds_1341) {
			/*
			 * For the above to be true, DS1341 also has to have
			 * ECLK bit set to 0
			 */
			ds1307->regs[1] &= ~DS1341_BIT_ECLK;

			/*
			 * Let's set additionale RTC bits to
			 * facilitate maximum power saving.
			 */
			ds1307->regs[0] |=  DS1341_BIT_DOSF;
			ds1307->regs[0] &= ~DS1341_BIT_EGFIL;

			i2c_smbus_write_byte_data(client, DS1337_REG_CONTROL,
						  ds1307->regs[0]);
			i2c_smbus_write_byte_data(client, DS1337_REG_STATUS,
						  ds1307->regs[1]);
		}


		/* oscillator fault?  clear flag, and warn */
		if (ds1307->regs[1] & DS1337_BIT_OSF) {
			i2c_smbus_write_byte_data(client, DS1337_REG_STATUS,
				ds1307->regs[1] & ~DS1337_BIT_OSF);
			dev_warn(&client->dev, "SET TIME!\n");
		}

	default:
		break;
	}

read_rtc:
	/* read RTC registers */
	tmp = ds1307->read_block_data(client, ds1307->offset, 8, buf);
	if (tmp != 8) {
		dev_dbg(&client->dev, "read error %d\n", tmp);
		err = -EIO;
		goto exit;
	}

	/* Configure clock using OF data if available */
	if (IS_ENABLED(CONFIG_OFDEVICE) && np) {
		u8 control = ds1307->regs[DS1307_REG_CONTROL];
		u32 rate = 0;

		if (of_property_read_bool(np, "ext-clock-input"))
			control |= DS1308_BIT_ECLK;
		else
			control &= ~DS1308_BIT_ECLK;

		if (of_property_read_bool(np, "ext-clock-output"))
			control |= DS1307_BIT_SQWE;
		else
			control &= ~DS1307_BIT_SQWE;

		if (of_property_read_bool(np, "ext-clock-bb"))
			control |= DS1308_BIT_BBCLK;
		else
			control &= ~DS1308_BIT_BBCLK;

		control &= ~(DS1307_BIT_RS1 | DS1307_BIT_RS0);
		of_property_read_u32(np, "ext-clock-rate", &rate);
		switch (rate) {
			default:
			case 1:     control |= 0; break;
			case 50:    control |= 1; break;
			case 60:    control |= 2; break;
			case 4096:  control |= 1; break;
			case 8192:  control |= 2; break;
			case 32768: control |= 3; break;
		}
		dev_dbg(&client->dev, "control reg: 0x%02x\n", control);

		if (ds1307->regs[DS1307_REG_CONTROL] != control) {
			i2c_smbus_write_byte_data(client, DS1307_REG_CONTROL, control);
			ds1307->regs[DS1307_REG_CONTROL]  = control;
		}
	}

	/*
	 * minimal sanity checking; some chips (like DS1340) don't
	 * specify the extra bits as must-be-zero, but there are
	 * still a few values that are clearly out-of-range.
	 */
	tmp = ds1307->regs[DS1307_REG_SECS];
	switch (ds1307->type) {
	case ds_1307:
		/* clock halted?  turn it on, so clock can tick. */
		if (tmp & DS1307_BIT_CH) {
			i2c_smbus_write_byte_data(client, DS1307_REG_SECS, 0);
			dev_warn(&client->dev, "SET TIME!\n");
			goto read_rtc;
		}
		break;
	case ds_1338:
		/* clock halted?  turn it on, so clock can tick. */
		if (tmp & DS1307_BIT_CH)
			i2c_smbus_write_byte_data(client, DS1307_REG_SECS, 0);

		/* oscillator fault?  clear flag, and warn */
		if (ds1307->regs[DS1307_REG_CONTROL] & DS1338_BIT_OSF) {
			i2c_smbus_write_byte_data(client, DS1307_REG_CONTROL,
					ds1307->regs[DS1307_REG_CONTROL]
					& ~DS1338_BIT_OSF);
			dev_warn(&client->dev, "SET TIME!\n");
			goto read_rtc;
		}
		break;
	default:
		break;
	}

	tmp = ds1307->regs[DS1307_REG_HOUR];
	switch (ds1307->type) {
	default:
		if (!(tmp & DS1307_BIT_12HR))
			break;

		/*
		 * Be sure we're in 24 hour mode.  Multi-master systems
		 * take note...
		 */
		tmp = bcd2bin(tmp & 0x1f);
		if (tmp == 12)
			tmp = 0;
		if (ds1307->regs[DS1307_REG_HOUR] & DS1307_BIT_PM)
			tmp += 12;
		i2c_smbus_write_byte_data(client,
				ds1307->offset + DS1307_REG_HOUR,
				bin2bcd(tmp));
	}

	ds1307->rtc.ops = &ds13xx_rtc_ops;
	ds1307->rtc.dev = dev;

	err = rtc_register(&ds1307->rtc);

exit:
	if (err)
		free(ds1307);
	return err;
}

static struct driver_d ds1307_driver = {
	.name	= "rtc-ds1307",
	.probe		= ds1307_probe,
	.id_table	= ds1307_id,
};
device_i2c_driver(ds1307_driver);
