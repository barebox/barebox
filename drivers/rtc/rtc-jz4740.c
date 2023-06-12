// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  JZ4740 SoC RTC driver
 *
 *  This code was ported from linux-3.15 kernel by Antony Pavlov.
 *
 *  Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
 *  Copyright (C) 2010, Paul Cercueil <paul@crapouillou.net>
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <io.h>
#include <rtc.h>
#include <linux/rtc.h>

#define JZ_REG_RTC_CTRL		0x00
#define JZ_REG_RTC_SEC		0x04
#define JZ_REG_RTC_SEC_ALARM	0x08
#define JZ_REG_RTC_REGULATOR	0x0C
#define JZ_REG_RTC_HIBERNATE	0x20
#define JZ_REG_RTC_SCRATCHPAD	0x34

#define JZ_RTC_CTRL_WRDY	BIT(7)

struct jz4740_rtc {
	struct rtc_device rtc;

	void __iomem *base;
};

static inline uint32_t jz4740_rtc_reg_read(struct jz4740_rtc *rtc, size_t reg)
{
	return readl(rtc->base + reg);
}

static int jz4740_rtc_wait_write_ready(struct jz4740_rtc *rtc)
{
	uint32_t ctrl;
	int timeout = 1000;

	do {
		ctrl = jz4740_rtc_reg_read(rtc, JZ_REG_RTC_CTRL);
	} while (!(ctrl & JZ_RTC_CTRL_WRDY) && --timeout);

	return timeout ? 0 : -EIO;
}

static inline int jz4740_rtc_reg_write(struct jz4740_rtc *rtc, size_t reg,
	uint32_t val)
{
	int ret;
	ret = jz4740_rtc_wait_write_ready(rtc);
	if (ret == 0)
		writel(val, rtc->base + reg);

	return ret;
}

static inline struct jz4740_rtc *to_jz4740_rtc_priv(struct rtc_device *rtcdev)
{
	return container_of(rtcdev, struct jz4740_rtc, rtc);
}

static int jz4740_rtc_read_time(struct rtc_device *rtcdev, struct rtc_time *time)
{
	struct jz4740_rtc *rtc = to_jz4740_rtc_priv(rtcdev);
	uint32_t secs, secs2;
	int timeout = 5;

	/* If the seconds register is read while it is updated, it can contain a
	 * bogus value. This can be avoided by making sure that two consecutive
	 * reads have the same value.
	 */
	secs = jz4740_rtc_reg_read(rtc, JZ_REG_RTC_SEC);
	secs2 = jz4740_rtc_reg_read(rtc, JZ_REG_RTC_SEC);

	while (secs != secs2 && --timeout) {
		secs = secs2;
		secs2 = jz4740_rtc_reg_read(rtc, JZ_REG_RTC_SEC);
	}

	if (timeout == 0)
		return -EIO;

	rtc_time_to_tm(secs, time);

	return rtc_valid_tm(time);
}

static int jz4740_rtc_set_time(struct rtc_device *rtcdev, struct rtc_time *t)
{
	struct jz4740_rtc *rtc = to_jz4740_rtc_priv(rtcdev);
	unsigned long secs;

	rtc_tm_to_time(t, &secs);

	return jz4740_rtc_reg_write(rtc, JZ_REG_RTC_SEC, secs);
}

static struct rtc_class_ops jz4740_rtc_ops = {
	.read_time	= jz4740_rtc_read_time,
	.set_time	= jz4740_rtc_set_time,
};

static int jz4740_rtc_probe(struct device *dev)
{
	struct resource *iores;
	int ret;
	struct jz4740_rtc *rtc;
	uint32_t scratchpad;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	base = IOMEM(iores->start);

	rtc = xzalloc(sizeof(*rtc));

	rtc->base = base;
	rtc->rtc.ops = &jz4740_rtc_ops;
	rtc->rtc.dev = dev;

	ret = rtc_register(&rtc->rtc);
	if (ret) {
		dev_err(dev, "Failed to register rtc device: %d\n", ret);
		return ret;
	}

	scratchpad = jz4740_rtc_reg_read(rtc, JZ_REG_RTC_SCRATCHPAD);
	if (scratchpad != 0x12345678) {
		ret = jz4740_rtc_reg_write(rtc, JZ_REG_RTC_SCRATCHPAD, 0x12345678);
		ret = jz4740_rtc_reg_write(rtc, JZ_REG_RTC_SEC, 0);
		if (ret) {
			dev_err(dev, "Could not write write to RTC registers\n");
			return ret;
		}
	}

	return 0;
}

static __maybe_unused struct of_device_id jz4740_rtc_dt_ids[] = {
	{
		.compatible = "ingenic,jz4740-rtc",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, jz4740_rtc_dt_ids);

static struct driver jz4740_rtc_driver = {
	.name  = "jz4740-rtc",
	.probe	 = jz4740_rtc_probe,
	.of_compatible = DRV_OF_COMPAT(jz4740_rtc_dt_ids),
};
device_platform_driver(jz4740_rtc_driver);
