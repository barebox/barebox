// SPDX-License-Identifier: GPL-2.0
/*
 * An I2C driver for the PCF85063 RTC
 * Copyright 2014 Rose Technology
 *
 * Author: Søren Andersen <san@rosetechnology.dk>
 * Maintainers: http://www.nslu2-linux.org/
 *
 * Copyright (C) 2019 Micro Crystal AG
 * Author: Alexandre Belloni <alexandre.belloni@bootlin.com>
 */
#include <linux/clk-provider.h>
#include <i2c/i2c.h>
#include <linux/bcd.h>
#include <linux/device.h>
#include <linux/rtc.h>
#include <of.h>
#include <linux/regmap.h>

/*
 * Information for this driver was pulled from the following datasheets.
 *
 *  https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf
 *  https://www.nxp.com/docs/en/data-sheet/PCF85063TP.pdf
 *
 *  PCF85063A -- Rev. 7 — 30 March 2018
 *  PCF85063TP -- Rev. 4 — 6 May 2015
 *
 *  https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-8263-C7_App-Manual.pdf
 *  RV8263 -- Rev. 1.0 — January 2019
 */

#define PCF85063_REG_CTRL1		0x00 /* status */
#define PCF85063_REG_CTRL1_CAP_SEL	BIT(0)
#define PCF85063_REG_CTRL1_STOP		BIT(5)
#define PCF85063_REG_CTRL1_EXT_TEST	BIT(7)

#define PCF85063_REG_CTRL2		0x01
#define PCF85063_CTRL2_AF		BIT(6)
#define PCF85063_CTRL2_AIE		BIT(7)

#define PCF85063_REG_OFFSET		0x02
#define PCF85063_OFFSET_SIGN_BIT	6	/* 2's complement sign bit */
#define PCF85063_OFFSET_MODE		BIT(7)
#define PCF85063_OFFSET_STEP0		4340
#define PCF85063_OFFSET_STEP1		4069

#define PCF85063_REG_CLKO_F_MASK	0x07 /* frequency mask */
#define PCF85063_REG_CLKO_F_32768HZ	0x00
#define PCF85063_REG_CLKO_F_OFF		0x07

#define PCF85063_REG_RAM		0x03

#define PCF85063_REG_SC			0x04 /* datetime */
#define PCF85063_REG_SC_OS		0x80

#define PCF85063_REG_ALM_S		0x0b
#define PCF85063_AEN			BIT(7)

struct pcf85063_config {
	struct regmap_config regmap;
	unsigned force_cap_7000:1;
};

struct pcf85063 {
	struct rtc_device	rtc;
	struct regmap		*regmap;
#ifdef CONFIG_COMMON_CLK
	struct clk_hw		clkout_hw;
#endif
};

static inline struct pcf85063 *to_pcf85063_priv(struct rtc_device *rtcdev)
{
	return container_of(rtcdev, struct pcf85063, rtc);
}

static int pcf85063_rtc_read_time(struct rtc_device *rtcdev, struct rtc_time *tm)
{
	struct pcf85063 *pcf85063 = to_pcf85063_priv(rtcdev);
	int rc;
	u8 regs[7];

	/*
	 * while reading, the time/date registers are blocked and not updated
	 * anymore until the access is finished. To not lose a second
	 * event, the access must be finished within one second. So, read all
	 * time/date registers in one turn.
	 */
	rc = regmap_bulk_read(pcf85063->regmap, PCF85063_REG_SC, regs,
			      sizeof(regs));
	if (rc)
		return rc;

	/* if the clock has lost its power it makes no sense to use its time */
	if (regs[0] & PCF85063_REG_SC_OS) {
		dev_warn(pcf85063->rtc.dev, "Power loss detected, invalid time\n");
		return -EINVAL;
	}

	tm->tm_sec = bcd2bin(regs[0] & 0x7F);
	tm->tm_min = bcd2bin(regs[1] & 0x7F);
	tm->tm_hour = bcd2bin(regs[2] & 0x3F); /* rtc hr 0-23 */
	tm->tm_mday = bcd2bin(regs[3] & 0x3F);
	tm->tm_wday = regs[4] & 0x07;
	tm->tm_mon = bcd2bin(regs[5] & 0x1F) - 1; /* rtc mn 1-12 */
	tm->tm_year = bcd2bin(regs[6]);
	tm->tm_year += 100;

	return 0;
}

static int pcf85063_rtc_set_time(struct rtc_device *rtcdev, struct rtc_time *tm)
{
	struct pcf85063 *pcf85063 = to_pcf85063_priv(rtcdev);
	int rc;
	u8 regs[7];

	/*
	 * to accurately set the time, reset the divider chain and keep it in
	 * reset state until all time/date registers are written
	 */
	rc = regmap_update_bits(pcf85063->regmap, PCF85063_REG_CTRL1,
				PCF85063_REG_CTRL1_EXT_TEST |
				PCF85063_REG_CTRL1_STOP,
				PCF85063_REG_CTRL1_STOP);
	if (rc)
		return rc;

	/* hours, minutes and seconds */
	regs[0] = bin2bcd(tm->tm_sec) & 0x7F; /* clear OS flag */

	regs[1] = bin2bcd(tm->tm_min);
	regs[2] = bin2bcd(tm->tm_hour);

	/* Day of month, 1 - 31 */
	regs[3] = bin2bcd(tm->tm_mday);

	/* Day, 0 - 6 */
	regs[4] = tm->tm_wday & 0x07;

	/* month, 1 - 12 */
	regs[5] = bin2bcd(tm->tm_mon + 1);

	/* year and century */
	regs[6] = bin2bcd(tm->tm_year - 100);

	/* write all registers at once */
	rc = regmap_bulk_write(pcf85063->regmap, PCF85063_REG_SC,
			       regs, sizeof(regs));
	if (rc)
		return rc;

	/*
	 * Write the control register as a separate action since the size of
	 * the register space is different between the PCF85063TP and
	 * PCF85063A devices.  The rollover point can not be used.
	 */
	return regmap_update_bits(pcf85063->regmap, PCF85063_REG_CTRL1,
				  PCF85063_REG_CTRL1_STOP, 0);
}

static const struct rtc_class_ops pcf85063_rtc_ops = {
	.read_time	= pcf85063_rtc_read_time,
	.set_time	= pcf85063_rtc_set_time,
};

static int pcf85063_nvmem_read(void *priv, unsigned int offset,
			       void *val, size_t bytes)
{
	return regmap_read(priv, PCF85063_REG_RAM, val);
}

static int pcf85063_nvmem_write(void *priv, unsigned int offset,
				const void *val, size_t bytes)
{
	return regmap_write(priv, PCF85063_REG_RAM, *(u8 *)val);
}

static int pcf85063_load_capacitance(struct pcf85063 *pcf85063,
				     const struct device_node *np,
				     unsigned int force_cap)
{
	u32 load = 7000;
	u8 reg = 0;

	if (force_cap)
		load = force_cap;
	else
		of_property_read_u32(np, "quartz-load-femtofarads", &load);

	switch (load) {
	default:
		dev_warn(pcf85063->rtc.dev, "Unknown quartz-load-femtofarads value: %d. Assuming 7000",
			 load);
		fallthrough;
	case 7000:
		break;
	case 12500:
		reg = PCF85063_REG_CTRL1_CAP_SEL;
		break;
	}

	return regmap_update_bits(pcf85063->regmap, PCF85063_REG_CTRL1,
				  PCF85063_REG_CTRL1_CAP_SEL, reg);
}

#ifdef CONFIG_COMMON_CLK
/*
 * Handling of the clkout
 */

#define clkout_hw_to_pcf85063(_hw) container_of(_hw, struct pcf85063, clkout_hw)

static int clkout_rates[] = {
	32768,
	16384,
	8192,
	4096,
	2048,
	1024,
	1,
	0
};

static unsigned long pcf85063_clkout_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct pcf85063 *pcf85063 = clkout_hw_to_pcf85063(hw);
	unsigned int buf;
	int ret = regmap_read(pcf85063->regmap, PCF85063_REG_CTRL2, &buf);

	if (ret < 0)
		return 0;

	buf &= PCF85063_REG_CLKO_F_MASK;
	return clkout_rates[buf];
}

static long pcf85063_clkout_round_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long *prate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clkout_rates); i++)
		if (clkout_rates[i] <= rate)
			return clkout_rates[i];

	return 0;
}

static int pcf85063_clkout_set_rate(struct clk_hw *hw, unsigned long rate,
				    unsigned long parent_rate)
{
	struct pcf85063 *pcf85063 = clkout_hw_to_pcf85063(hw);
	int i;

	for (i = 0; i < ARRAY_SIZE(clkout_rates); i++)
		if (clkout_rates[i] == rate)
			return regmap_update_bits(pcf85063->regmap,
				PCF85063_REG_CTRL2,
				PCF85063_REG_CLKO_F_MASK, i);

	return -EINVAL;
}

static int pcf85063_clkout_control(struct clk_hw *hw, bool enable)
{
	struct pcf85063 *pcf85063 = clkout_hw_to_pcf85063(hw);
	unsigned int buf;
	int ret;

	ret = regmap_read(pcf85063->regmap, PCF85063_REG_CTRL2, &buf);
	if (ret < 0)
		return ret;
	buf &= PCF85063_REG_CLKO_F_MASK;

	if (enable) {
		if (buf == PCF85063_REG_CLKO_F_OFF)
			buf = PCF85063_REG_CLKO_F_32768HZ;
		else
			return 0;
	} else {
		if (buf != PCF85063_REG_CLKO_F_OFF)
			buf = PCF85063_REG_CLKO_F_OFF;
		else
			return 0;
	}

	return regmap_update_bits(pcf85063->regmap, PCF85063_REG_CTRL2,
					PCF85063_REG_CLKO_F_MASK, buf);
}

static int pcf85063_clkout_prepare(struct clk_hw *hw)
{
	return pcf85063_clkout_control(hw, 1);
}

static void pcf85063_clkout_unprepare(struct clk_hw *hw)
{
	pcf85063_clkout_control(hw, 0);
}

static int pcf85063_clkout_is_prepared(struct clk_hw *hw)
{
	struct pcf85063 *pcf85063 = clkout_hw_to_pcf85063(hw);
	unsigned int buf;
	int ret = regmap_read(pcf85063->regmap, PCF85063_REG_CTRL2, &buf);

	if (ret < 0)
		return 0;

	return (buf & PCF85063_REG_CLKO_F_MASK) != PCF85063_REG_CLKO_F_OFF;
}

static const struct clk_ops pcf85063_clkout_ops = {
	.enable = pcf85063_clkout_prepare,
	.disable = pcf85063_clkout_unprepare,
	.is_enabled = pcf85063_clkout_is_prepared,
	.recalc_rate = pcf85063_clkout_recalc_rate,
	.round_rate = pcf85063_clkout_round_rate,
	.set_rate = pcf85063_clkout_set_rate,
};

static struct clk *pcf85063_clkout_register_clk(struct pcf85063 *pcf85063)
{
	struct clk *clk;
	struct clk_init_data init;
	struct device_node *node = dev_of_node(pcf85063->rtc.dev);
	struct device_node *fixed_clock;

	fixed_clock = of_get_child_by_name(node, "clock");
	if (fixed_clock) {
		/*
		 * skip registering square wave clock when a fixed
		 * clock has been registered. The fixed clock is
		 * registered automatically when being referenced.
		 */
		of_node_put(fixed_clock);
		return NULL;
	}

	init.name = "pcf85063-clkout";
	init.ops = &pcf85063_clkout_ops;
	init.flags = 0;
	init.parent_names = NULL;
	init.num_parents = 0;
	pcf85063->clkout_hw.init = &init;

	/* optional override of the clockname */
	of_property_read_string(node, "clock-output-names", &init.name);

	/* register the clock */
	clk = clk_register(pcf85063->rtc.dev, &pcf85063->clkout_hw);

	if (!IS_ERR(clk))
		of_clk_add_provider(node, of_clk_src_simple_get, clk);

	return clk;
}
#endif

static const struct pcf85063_config config_pcf85063 = {
	.regmap = {
		.reg_bits = 8,
		.val_bits = 8,
		.max_register = 0x0a,
	},
};

static const struct pcf85063_config config_pcf85063tp = {
	.regmap = {
		.reg_bits = 8,
		.val_bits = 8,
		.max_register = 0x0a,
	},
};

static const struct pcf85063_config config_pcf85063a = {
	.regmap = {
		.reg_bits = 8,
		.val_bits = 8,
		.max_register = 0x11,
	},
};

static const struct pcf85063_config config_rv8263 = {
	.regmap = {
		.reg_bits = 8,
		.val_bits = 8,
		.max_register = 0x11,
	},
	.force_cap_7000 = 1,
};

static int pcf85063_probe(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pcf85063 *pcf85063;
	unsigned int tmp;
	int err;
	const struct pcf85063_config *config;
	struct nvmem_config nvmem_cfg = {
		.name = "pcf85063_nvram",
		.reg_read = pcf85063_nvmem_read,
		.reg_write = pcf85063_nvmem_write,
		.size = 1,
	};

	dev_dbg(&client->dev, "%s\n", __func__);

	pcf85063 = devm_kzalloc(&client->dev, sizeof(struct pcf85063),
				GFP_KERNEL);
	if (!pcf85063)
		return -ENOMEM;

	config = device_get_match_data(dev);
	if (!config)
		return -ENODEV;

	pcf85063->regmap = regmap_init_i2c(client, &config->regmap);
	if (IS_ERR(pcf85063->regmap))
		return PTR_ERR(pcf85063->regmap);

	i2c_set_clientdata(client, pcf85063);

	err = regmap_read(pcf85063->regmap, PCF85063_REG_CTRL1, &tmp);
	if (err) {
		dev_err(&client->dev, "RTC chip is not present\n");
		return err;
	}

	pcf85063->rtc.ops = &pcf85063_rtc_ops;
	pcf85063->rtc.dev = dev;

	err = pcf85063_load_capacitance(pcf85063, client->dev.of_node,
					config->force_cap_7000 ? 7000 : 0);
	if (err < 0)
		dev_warn(&client->dev, "failed to set xtal load capacitance: %d",
			 err);


	nvmem_cfg.priv = pcf85063->regmap;
	rtc_nvmem_register(&pcf85063->rtc, &nvmem_cfg);

#ifdef CONFIG_COMMON_CLK
	/* register clk in common clk framework */
	pcf85063_clkout_register_clk(pcf85063);
#endif

	return rtc_register(&pcf85063->rtc);
}

static const struct of_device_id pcf85063_of_match[] = {
	{ .compatible = "nxp,pca85073a", .data = &config_pcf85063a },
	{ .compatible = "nxp,pcf85063", .data = &config_pcf85063 },
	{ .compatible = "nxp,pcf85063tp", .data = &config_pcf85063tp },
	{ .compatible = "nxp,pcf85063a", .data = &config_pcf85063a },
	{ .compatible = "microcrystal,rv8263", .data = &config_rv8263 },
	{}
};
MODULE_DEVICE_TABLE(of, pcf85063_of_match);

static struct driver pcf85063_driver = {
	.name	= "rtc-pcf85063",
	.of_match_table = pcf85063_of_match,
	.probe		= pcf85063_probe,
};
device_i2c_driver(pcf85063_driver);

MODULE_AUTHOR("Søren Andersen <san@rosetechnology.dk>");
MODULE_DESCRIPTION("PCF85063 RTC driver");
MODULE_LICENSE("GPL");
