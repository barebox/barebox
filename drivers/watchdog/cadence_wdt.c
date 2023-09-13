// SPDX-License-Identifier: GPL-2.0+
/*
 * Cadence WDT driver - Used by Xilinx Zynq
 *
 * Copyright (C) 2010 - 2014 Xilinx, Inc.
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <restart.h>
#include <watchdog.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/reset.h>

/* Supports 1 - 516 sec */
#define CDNS_WDT_MAX_TIMEOUT	516

/* Restart key */
#define CDNS_WDT_RESTART_KEY 0x00001999

/* Counter register access key */
#define CDNS_WDT_REGISTER_ACCESS_KEY 0x00920000

/* Counter value divisor */
#define CDNS_WDT_COUNTER_VALUE_DIVISOR 0x1000

/* Clock prescaler value and selection */
#define CDNS_WDT_PRESCALE_64	64
#define CDNS_WDT_PRESCALE_512	512
#define CDNS_WDT_PRESCALE_4096	4096
#define CDNS_WDT_PRESCALE_SELECT_64	1
#define CDNS_WDT_PRESCALE_SELECT_512	2
#define CDNS_WDT_PRESCALE_SELECT_4096	3

/* Input clock frequency */
#define CDNS_WDT_CLK_10MHZ	10000000
#define CDNS_WDT_CLK_75MHZ	75000000

/* Counter maximum value */
#define CDNS_WDT_COUNTER_MAX 0xFFF

/**
 * struct cdns_wdt - Watchdog device structure
 * @regs: baseaddress of device
 * @clk: struct clk * of a clock source
 * @prescaler: for saving prescaler value
 * @ctrl_clksel: counter clock prescaler selection
 * @cdns_wdt_device: watchdog device structure
 *
 * Structure containing parameters specific to cadence watchdog.
 */
struct cdns_wdt {
	void __iomem		*regs;
	struct clk		*clk;
	u32			prescaler;
	u32			ctrl_clksel;
	struct watchdog		cdns_wdt_device;
	unsigned		timeout;
};

static inline struct cdns_wdt *to_cdns_wdt(struct watchdog *wdd)
{
	return container_of(wdd, struct cdns_wdt, cdns_wdt_device);
}

/* Write access to Registers */
static inline void cdns_wdt_writereg(struct cdns_wdt *wdt, u32 offset, u32 val)
{
	writel_relaxed(val, wdt->regs + offset);
}

/*************************Register Map**************************************/

/* Register Offsets for the WDT */
#define CDNS_WDT_ZMR_OFFSET	0x0	/* Zero Mode Register */
#define CDNS_WDT_CCR_OFFSET	0x4	/* Counter Control Register */
#define CDNS_WDT_RESTART_OFFSET	0x8	/* Restart Register */
#define CDNS_WDT_SR_OFFSET	0xC	/* Status Register */

/*
 * Zero Mode Register - This register controls how the time out is indicated
 * and also contains the access code to allow writes to the register (0xABC).
 */
#define CDNS_WDT_ZMR_WDEN_MASK	0x00000001 /* Enable the WDT */
#define CDNS_WDT_ZMR_RSTEN_MASK	0x00000002 /* Enable the reset output */
#define CDNS_WDT_ZMR_IRQEN_MASK	0x00000004 /* Enable IRQ output */
#define CDNS_WDT_ZMR_RSTLEN_16	0x00000030 /* Reset pulse of 16 pclk cycles */
#define CDNS_WDT_ZMR_ZKEY_VAL	0x00ABC000 /* Access key, 0xABC << 12 */
/*
 * Counter Control register - This register controls how fast the timer runs
 * and the reset value and also contains the access code to allow writes to
 * the register.
 */
#define CDNS_WDT_CCR_CRV_MASK	0x00003FFC /* Counter reset value */

/**
 * cdns_wdt_stop - Stop the watchdog.
 *
 * @wdt: cadence watchdog device
 *
 * Read the contents of the ZMR register, clear the WDEN bit
 * in the register and set the access key for successful write.
 */
static void cdns_wdt_stop(struct cdns_wdt *wdt)
{
	cdns_wdt_writereg(wdt, CDNS_WDT_ZMR_OFFSET,
			  CDNS_WDT_ZMR_ZKEY_VAL & (~CDNS_WDT_ZMR_WDEN_MASK));
}

/**
 * cdns_wdt_reload - Reload the watchdog timer (i.e. pat the watchdog).
 *
 * @wdt: cadence watchdog device
 *
 * Write the restart key value (0x00001999) to the restart register.
 */
static void cdns_wdt_reload(struct cdns_wdt *wdt)
{
	cdns_wdt_writereg(wdt, CDNS_WDT_RESTART_OFFSET,
			  CDNS_WDT_RESTART_KEY);
}

/**
 * cdns_wdt_start - Enable and start the watchdog.
 *
 * @wdt: cadence watchdog device
 * @timeout: new timeout
 *
 * The counter value is calculated according to the formula:
 *		calculated count = (timeout * clock) / prescaler + 1.
 * The calculated count is divided by 0x1000 to obtain the field value
 * to write to counter control register.
 * Clears the contents of prescaler and counter reset value. Sets the
 * prescaler to 4096 and the calculated count and access key
 * to write to CCR Register.
 * Sets the WDT (WDEN bit) and either the Reset signal(RSTEN bit)
 * or Interrupt signal(IRQEN) with a specified cycles and the access
 * key to write to ZMR Register.
 */
static void cdns_wdt_start(struct cdns_wdt *wdt, unsigned timeout)
{
	unsigned int data = 0;
	unsigned short count;
	unsigned long clock_f = clk_get_rate(wdt->clk);

	/*
	 * Counter value divisor to obtain the value of
	 * counter reset to be written to control register.
	 */
	count = (timeout * (clock_f / wdt->prescaler)) /
		 CDNS_WDT_COUNTER_VALUE_DIVISOR + 1;

	if (count > CDNS_WDT_COUNTER_MAX)
		count = CDNS_WDT_COUNTER_MAX;

	cdns_wdt_writereg(wdt, CDNS_WDT_ZMR_OFFSET,
			  CDNS_WDT_ZMR_ZKEY_VAL);

	count = (count << 2) & CDNS_WDT_CCR_CRV_MASK;

	/* Write counter access key first to be able write to register */
	data = count | CDNS_WDT_REGISTER_ACCESS_KEY | wdt->ctrl_clksel;
	cdns_wdt_writereg(wdt, CDNS_WDT_CCR_OFFSET, data);
	data = CDNS_WDT_ZMR_WDEN_MASK | CDNS_WDT_ZMR_RSTLEN_16 |
	       CDNS_WDT_ZMR_ZKEY_VAL;

	/* Reset on timeout regardless of what's specified in device tree. */
	data |= CDNS_WDT_ZMR_RSTEN_MASK;
	data &= ~CDNS_WDT_ZMR_IRQEN_MASK;

	cdns_wdt_writereg(wdt, CDNS_WDT_ZMR_OFFSET, data);
	cdns_wdt_writereg(wdt, CDNS_WDT_RESTART_OFFSET,
			  CDNS_WDT_RESTART_KEY);
}

/**
 * cdns_wdt_settimeout - Set a new timeout value for the watchdog device.
 *
 * @wdd: watchdog device
 * @new_time: new timeout value that needs to be set
 * Return: 0 on success
 *
 * Update the watchdog device timeout with new value which is used when
 * cdns_wdt_start is called.
 */
static int cdns_wdt_settimeout(struct watchdog *wdd,
			       unsigned int new_time)
{
	struct cdns_wdt *wdt = to_cdns_wdt(wdd);

	if (new_time > wdd->timeout_max)
		return -EINVAL;

	if (new_time == 0) {
		cdns_wdt_stop(wdt);
	} else if (wdt->timeout != new_time) {
		cdns_wdt_start(wdt, new_time);
		wdt->timeout = new_time;
	} else {
		cdns_wdt_reload(wdt);
	}

	return 0;
}

/************************Platform Operations*****************************/
/**
 * cdns_wdt_probe - Probe call for the device.
 *
 * @pdev: handle to the platform device structure.
 * Return: 0 on success, negative error otherwise.
 *
 * It does all the memory allocation and registration for the device.
 */
static int cdns_wdt_probe(struct device *dev)
{
	unsigned long clock_f;
	struct cdns_wdt *wdt;
	struct resource *res;
	struct watchdog *cdns_wdt_device;

	wdt = xzalloc(sizeof(*wdt));

	cdns_wdt_device = &wdt->cdns_wdt_device;
	cdns_wdt_device->name = "cdns_wdt";
	cdns_wdt_device->hwdev = dev;
	cdns_wdt_device->set_timeout = cdns_wdt_settimeout;
	cdns_wdt_device->timeout_max = CDNS_WDT_MAX_TIMEOUT;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	wdt->regs = IOMEM(res->start);

	/* We don't service interrupts in barebox, so a watchdog that doesn't
	 * reset the system isn't a useful thing to register
	 */
	if (!of_property_read_bool(dev->of_node, "reset-on-timeout"))
		dev_notice(dev, "proceeding as if reset-on-timeout was set\n");

	wdt->clk = clk_get_enabled(dev, NULL);
	if (IS_ERR(wdt->clk))
		return dev_err_probe(dev, PTR_ERR(wdt->clk),
				     "input clock not found\n");

	clock_f = clk_get_rate(wdt->clk);
	if (clock_f <= CDNS_WDT_CLK_75MHZ) {
		wdt->prescaler = CDNS_WDT_PRESCALE_512;
		wdt->ctrl_clksel = CDNS_WDT_PRESCALE_SELECT_512;
	} else {
		wdt->prescaler = CDNS_WDT_PRESCALE_4096;
		wdt->ctrl_clksel = CDNS_WDT_PRESCALE_SELECT_4096;
	}

	return watchdog_register(cdns_wdt_device);
}

static const struct of_device_id cdns_wdt_of_match[] = {
	{ .compatible = "cdns,wdt-r1p2", },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, cdns_wdt_of_match);

static struct driver cdns_wdt_driver = {
	.name		= "cdns-wdt",
	.probe		= cdns_wdt_probe,
	.of_match_table	= cdns_wdt_of_match,
};
device_platform_driver(cdns_wdt_driver);

MODULE_AUTHOR("Xilinx, Inc.");
MODULE_DESCRIPTION("Watchdog driver for Cadence WDT");
MODULE_LICENSE("GPL");
