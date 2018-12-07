/*
 * mc13xxx_adc
 *
 * Copyright (c) 2018 Zodiac Inflight Innovation
 * Author: Andrey Gusakov <andrey.gusakov@cogentembedded.com>
 * Based on the code of analogous driver from Linux:
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2009 Sascha Hauer, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <aiodev.h>
#include <mfd/mc13xxx.h>
#include <linux/err.h>

#define MC13XXX_ADC0_LICELLCON		(1 << 0)
#define MC13XXX_ADC0_CHRGICON		(1 << 1)
#define MC13XXX_ADC0_BATICON		(1 << 2)
#define MC13XXX_ADC0_BUFEN		(1 << 3)
#define MC13XXX_ADC0_ADIN7SEL_DIE	(1 << 4)
#define MC13XXX_ADC0_ADIN7SEL_UID	(2 << 4)
#define MC13XXX_ADC0_ADREFEN		(1 << 10)
#define MC13XXX_ADC0_TSMOD0		(1 << 12)
#define MC13XXX_ADC0_TSMOD1		(1 << 13)
#define MC13XXX_ADC0_TSMOD2		(1 << 14)
#define MC13XXX_ADC0_ADINC1		(1 << 16)
#define MC13XXX_ADC0_ADINC2		(1 << 17)

#define MC13XXX_ADC0_TSMOD_MASK		(MC13XXX_ADC0_TSMOD0 | \
					MC13XXX_ADC0_TSMOD1 | \
					MC13XXX_ADC0_TSMOD2)

#define MC13XXX_ADC0_CONFIG_MASK	(MC13XXX_ADC0_TSMOD_MASK | \
					MC13XXX_ADC0_LICELLCON | \
					MC13XXX_ADC0_CHRGICON | \
					MC13XXX_ADC0_BATICON)

#define MC13XXX_ADC1_ADEN		(1 << 0)
#define MC13XXX_ADC1_RAND		(1 << 1)
#define MC13XXX_ADC1_ADSEL		(1 << 3)
#define MC13XXX_ADC1_CHAN0_SHIFT	5
#define MC13XXX_ADC1_CHAN1_SHIFT	8
#define MC13XXX_ADC1_ASC		(1 << 20)
#define MC13XXX_ADC1_ADTRIGIGN		(1 << 21)

struct mc13xx_adc_data {
	struct mc13xxx *mc_dev;

	struct aiodevice aiodev;
	struct aiochannel *aiochan;
};

static inline struct mc13xx_adc_data *
to_mc13xx_adc_data(struct aiochannel *chan)
{
	return container_of(chan->aiodev, struct mc13xx_adc_data, aiodev);
}

static int mc13xxx_adc_do_conversion(struct mc13xxx *mc13xxx,
				     unsigned int channel,
				     unsigned int *sample)
{
	int i;
	int timeout = 100;
	u32 adc0, adc1, old_adc0;

	mc13xxx_reg_read(mc13xxx, MC13783_REG_ADC(0), &old_adc0);

	adc0 = MC13XXX_ADC0_ADINC1 | MC13XXX_ADC0_ADINC2 | MC13XXX_ADC0_BUFEN;
	adc1 = MC13XXX_ADC1_ADEN | MC13XXX_ADC1_ADTRIGIGN | MC13XXX_ADC1_ASC;

	/* Channels mapped through ADIN7:
	 * 7  - General purpose ADIN7
	 * 16 - UID
	 * 17 - Die temperature */
	if (channel > 7 && channel < 16) {
		adc1 |= MC13XXX_ADC1_ADSEL;
	} else if (channel == 16) {
		adc0 |= MC13XXX_ADC0_ADIN7SEL_UID;
		channel = 7;
	} else if (channel == 17) {
		adc0 |= MC13XXX_ADC0_ADIN7SEL_DIE;
		channel = 7;
	}

	adc0 |= old_adc0 & MC13XXX_ADC0_CONFIG_MASK;
	adc1 |= (channel & 0x7) << MC13XXX_ADC1_CHAN0_SHIFT;
	adc1 |= MC13XXX_ADC1_RAND;

	mc13xxx_reg_write(mc13xxx, MC13783_REG_ADC(0), adc0);
	mc13xxx_reg_write(mc13xxx, MC13783_REG_ADC(1), adc1);

	/* wait for completion. ASC will set to zero */
	do {
		mc13xxx_reg_read(mc13xxx, MC13783_REG_ADC(1), &adc1);
	} while ((adc1 & MC13XXX_ADC1_ASC) && (--timeout > 0));

	if (timeout == 0)
		return -ETIMEDOUT;

	for (i = 0; i < 4; ++i) {
		mc13xxx_reg_read(mc13xxx,
			MC13783_REG_ADC(2), &sample[i]);
	}

	return 0;
}

static int mc13xx_adc_read(struct aiochannel *chan, int *val)
{
	int i;
	int ret;
	int mc_type;
	unsigned int sample[4];
	struct mc13xx_adc_data *mc13xxx_adc;
	int acc = 0;
	int index = chan->index;

	mc13xxx_adc = to_mc13xx_adc_data(chan);
	mc_type = mc13xxx_type(mc13xxx_adc->mc_dev);

	/* add offset for all 8 channel devices becouse t and UID
	 * inputs are mapped to channels 16 and 17 */
	if ((mc_type != MC13783_TYPE) && (chan->index > 7))
		index += 8;

	ret = mc13xxx_adc_do_conversion(mc13xxx_adc->mc_dev, index, sample);
	if (ret < 0)
		goto err;

	for (i = 0; i < 4; i++) {
		acc += (sample[i] >> 2 & 0x3ff);
		acc += (sample[i] >> 14 & 0x3ff);
	}
	/* div 8 */
	acc = acc >> 3;

	if (index == 16) {
		/* UID */
		if (mc_type == MC13892_TYPE) {
			/* MC13892 have 1/2 divider
			 * input range is [0, 4.800V] */
			acc = DIV_ROUND_CLOSEST(acc * 4800, 1024);
		} else {
			/* MC13783 have 0.9 divider
			 *input range is [0, 2.555V] */
			acc = DIV_ROUND_CLOSEST(acc * 2555, 1024);
		}
	} else if (index == 17) {
		/* Die temperature */
		if (mc_type == MC13892_TYPE) {
			/* MC13892:
			 * Die Temperature Read Out Code at 25C 680
			 * Temperature change per LSB +0.4244C */
			acc = DIV_ROUND_CLOSEST(-2635920 + acc * 4244, 10);
		} else {
			/* MC13783:
			 * Die Temperature Read Out Code at 25C 282
			 * Temperature change per LSB -1.14C */
			acc = 346480 - 1140 * acc;
		}
	} else {
		/* GP input
		 * input range is [0, 2.3V], value has 10 bits */
		acc = DIV_ROUND_CLOSEST(acc * 2300, 1024);
	}

	*val = acc;
err:
	return ret;
}

int mc13xxx_adc_probe(struct device_d *dev, struct mc13xxx *mc_dev)
{
	int i;
	int ret;
	int chans;
	struct mc13xx_adc_data *mc13xxx_adc;

	mc13xxx_adc = xzalloc(sizeof(*mc13xxx_adc));

	if (mc13xxx_type(mc_dev) == MC13783_TYPE) {
		/* mc13783 has 16 channels */
		chans = 16 + 2;
	} else {
		chans = 8 + 2;
	}

	mc13xxx_adc->mc_dev = mc_dev;
	mc13xxx_adc->aiodev.num_channels = chans;
	mc13xxx_adc->aiochan = xmalloc(mc13xxx_adc->aiodev.num_channels *
			sizeof(*mc13xxx_adc->aiochan));
	mc13xxx_adc->aiodev.hwdev = dev;
	mc13xxx_adc->aiodev.channels =
		xmalloc(mc13xxx_adc->aiodev.num_channels *
			sizeof(mc13xxx_adc->aiodev.channels[0]));
	/* all channels are voltage inputs, expect last one */
	for (i = 0; i < chans - 1; i++) {
		mc13xxx_adc->aiodev.channels[i] = &mc13xxx_adc->aiochan[i];
		mc13xxx_adc->aiochan[i].unit = "mV";
	}
	/* temperature input */
	mc13xxx_adc->aiodev.channels[i] = &mc13xxx_adc->aiochan[i];
	mc13xxx_adc->aiochan[i].unit = "mC";

	mc13xxx_adc->aiodev.read = mc13xx_adc_read;

	ret = aiodevice_register(&mc13xxx_adc->aiodev);
	if (!ret)
		goto done;

	dev_err(dev, "Failed to register AIODEV: %d\n", ret);
	kfree(mc13xxx_adc);
done:
	return ret;
}
