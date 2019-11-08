// SPDX-License-Identifier: GPL-2.0-or-later
/***************************************************************************
 *   Copyright (C) 2006 by Hans Edgington <hans@edgington.nl>              *
 *   Copyright (C) 2007-2009 Hans de Goede <hdegoede@redhat.com>           *
 *   Copyright (C) 2010 Giel van Schijndel <me@mortis.eu>                  *
 *   Copyright (C) 2019 Ahmad Fatoum <a.fatoum@pengutronix.de>             *
 *                                                                         *
 ***************************************************************************/

#define pr_fmt(fmt) "f71808e_wdt: " fmt

#include <init.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <driver.h>
#include <watchdog.h>
#include <printk.h>
#include <reset_source.h>
#include <superio.h>
#include <common.h>

#define SIO_F71808FG_LD_WDT	0x07	/* Watchdog timer logical device */
#define SIO_UNLOCK_KEY		0x87	/* Key to enable Super-I/O */
#define SIO_LOCK_KEY		0xAA	/* Key to disable Super-I/O */

#define SIO_REG_LDSEL		0x07	/* Logical device select */
#define SIO_REG_DEVREV		0x22	/* Device revision */
#define SIO_REG_ROM_ADDR_SEL	0x27	/* ROM address select */
#define SIO_F81866_REG_PORT_SEL	0x27	/* F81866 Multi-Function Register */
#define SIO_REG_MFUNCT1		0x29	/* Multi function select 1 */
#define SIO_REG_MFUNCT2		0x2a	/* Multi function select 2 */
#define SIO_REG_MFUNCT3		0x2b	/* Multi function select 3 */
#define SIO_F81866_REG_GPIO1	0x2c	/* F81866 GPIO1 Enable Register */
#define SIO_REG_ENABLE		0x30	/* Logical device enable */
#define SIO_REG_ADDR		0x60	/* Logical device address (2 bytes) */

#define F71808FG_REG_WDO_CONF		0xf0
#define F71808FG_REG_WDT_CONF		0xf5
#define F71808FG_REG_WD_TIME		0xf6

#define F71808FG_FLAG_WDOUT_EN		7

#define F71808FG_FLAG_WDTMOUT_STS	6
#define F71808FG_FLAG_WD_EN		5
#define F71808FG_FLAG_WD_PULSE		4
#define F71808FG_FLAG_WD_UNIT		3

#define F81865_REG_WDO_CONF		0xfa
#define F81865_FLAG_WDOUT_EN		0

/* Default values */
#define WATCHDOG_MAX_TIMEOUT	(60 * 255)

enum pulse_width {
	PULSE_WIDTH_LEVEL, PULSE_WIDTH_1MS,
	PULSE_WIDTH_LOW, PULSE_WIDTH_MID, PULSE_WIDTH_HIGH
};

const char *pulse_width_names[] = { "level", "1", "25", "125", "5000" };
const char *pulse_width_names_f71868[] = { "level", "1", "30", "150", "6000" };

enum wdtrst_pin {
	WDTRST_PIN_56, WDTRST_PIN_63,
};

const char *f71862fg_pin_names[] = { "56", "63" };

enum chips { f71808fg, f71858fg, f71862fg, f71868, f71869, f71882fg, f71889fg,
	     f81865, f81866};

struct f71808e_wdt;

struct f71808e_variant_data {
	enum chips	type;
	void (*pinconf)(struct f71808e_wdt *wd);
};

struct f71808e_wdt {
	struct watchdog wdd;
	u16		sioaddr;
	const struct f71808e_variant_data *variant;
	unsigned int	timeout;
	u8		timer_val;	/* content for the wd_time register */
	char		minutes_mode;
	int		pulse_width;
	int		f71862fg_pin;
};

static inline struct f71808e_wdt *to_f71808e_wdt(struct watchdog *wdd)
{
	return container_of(wdd, struct f71808e_wdt, wdd);
}

static inline bool has_f81865_wdo_conf(struct f71808e_wdt *wd)
{
	return wd->variant->type == f81865 || wd->variant->type == f81866;
}

static inline void superio_enter(u16 base)
{
	/* according to the datasheet the key must be sent twice! */
	outb(SIO_UNLOCK_KEY, base);
	outb(SIO_UNLOCK_KEY, base);
}

static inline void superio_select(u16 base, int ld)
{
	outb(SIO_REG_LDSEL, base);
	outb(ld, base + 1);
}

static inline void superio_exit(u16 base)
{
	outb(SIO_LOCK_KEY, base);
}

static void f71808e_wdt_keepalive(struct f71808e_wdt *wd)
{
	superio_enter(wd->sioaddr);

	superio_select(wd->sioaddr, SIO_F71808FG_LD_WDT);

	if (wd->minutes_mode)
		/* select minutes for timer units */
		superio_set_bit(wd->sioaddr, F71808FG_REG_WDT_CONF,
				F71808FG_FLAG_WD_UNIT);
	else
		/* select seconds for timer units */
		superio_clear_bit(wd->sioaddr, F71808FG_REG_WDT_CONF,
				F71808FG_FLAG_WD_UNIT);

	/* Set timer value */
	superio_outb(wd->sioaddr, F71808FG_REG_WD_TIME,
		     wd->timer_val);

	superio_exit(wd->sioaddr);
}

static void f71808e_wdt_start(struct f71808e_wdt *wd)
{
	/* Make sure we don't die as soon as the watchdog is enabled below */
	f71808e_wdt_keepalive(wd);

	superio_enter(wd->sioaddr);

	superio_select(wd->sioaddr, SIO_F71808FG_LD_WDT);

	/* Watchdog pin configuration */
	wd->variant->pinconf(wd);

	superio_select(wd->sioaddr, SIO_F71808FG_LD_WDT);
	superio_set_bit(wd->sioaddr, SIO_REG_ENABLE, 0);

	if (has_f81865_wdo_conf(wd))
		superio_set_bit(wd->sioaddr, F81865_REG_WDO_CONF,
				F81865_FLAG_WDOUT_EN);
	else
		superio_set_bit(wd->sioaddr, F71808FG_REG_WDO_CONF,
				F71808FG_FLAG_WDOUT_EN);

	superio_set_bit(wd->sioaddr, F71808FG_REG_WDT_CONF,
			F71808FG_FLAG_WD_EN);

	if (wd->pulse_width > 0) {
		/* Select "pulse" output mode with given duration */
		u8 wdt_conf = superio_inb(wd->sioaddr, F71808FG_REG_WDT_CONF);

		/* Set WD_PSWIDTH bits (1:0) */
		wdt_conf = (wdt_conf & 0xfc) | (wd->pulse_width & 0x03);
		/* Set WD_PULSE to "pulse" mode */
		wdt_conf |= BIT(F71808FG_FLAG_WD_PULSE);

		superio_outb(wd->sioaddr, F71808FG_REG_WDT_CONF, wdt_conf);
	} else {
		/* Select "level" output mode */
		superio_clear_bit(wd->sioaddr, F71808FG_REG_WDT_CONF,
				  F71808FG_FLAG_WD_PULSE);
	}

	superio_exit(wd->sioaddr);
}

static void f71808e_wdt_stop(struct f71808e_wdt *wd)
{
	superio_enter(wd->sioaddr);

	superio_select(wd->sioaddr, SIO_F71808FG_LD_WDT);

	superio_clear_bit(wd->sioaddr, F71808FG_REG_WDT_CONF,
			  F71808FG_FLAG_WD_EN);

	superio_exit(wd->sioaddr);
}

static int f71808e_wdt_set_timeout(struct watchdog *wdd, unsigned int new_timeout)
{
	struct f71808e_wdt *wd = to_f71808e_wdt(wdd);

	if (!new_timeout) {
		f71808e_wdt_stop(wd);
		return 0;
	}

	if (wd->timeout != new_timeout) {
		if (new_timeout > 0xff) {
			wd->timer_val = DIV_ROUND_UP(new_timeout, 60);
			wd->minutes_mode = true;
		} else {
			wd->timer_val = new_timeout;
			wd->minutes_mode = false;
		}

		f71808e_wdt_start(wd);
		wd->timeout = new_timeout;
	}

	f71808e_wdt_keepalive(wd);
	return 0;
}

static int f71808e_wdt_init(struct f71808e_wdt *wd, struct device_d *dev)
{
	struct watchdog *wdd = &wd->wdd;
	const char * const *names = pulse_width_names;
	unsigned long wdt_conf;
	int ret;

	superio_enter(wd->sioaddr);

	superio_select(wd->sioaddr, SIO_F71808FG_LD_WDT);

	wdt_conf = superio_inb(wd->sioaddr, F71808FG_REG_WDT_CONF);

	superio_exit(wd->sioaddr);

	if (wd->variant->type == f71868)
		names = pulse_width_names_f71868;

	wd->pulse_width = PULSE_WIDTH_MID; /* either 125ms or 150ms */

	dev_add_param_enum(dev, "pulse_width_ms", NULL, NULL,
			   &wd->pulse_width, names,
			   ARRAY_SIZE(pulse_width_names),
			   wd);

	if (wd->variant->type == f71862fg) {
		wd->f71862fg_pin = WDTRST_PIN_63;

		dev_add_param_enum(dev, "wdtrst_pin", NULL, NULL,
				   &wd->f71862fg_pin, f71862fg_pin_names,
				   ARRAY_SIZE(f71862fg_pin_names),
				   wd);
	}

	wdd->hwdev		= dev;
	wdd->set_timeout	= &f71808e_wdt_set_timeout;
	wdd->timeout_max	= WATCHDOG_MAX_TIMEOUT;

	if (wdt_conf & BIT(F71808FG_FLAG_WDTMOUT_STS))
		reset_source_set_priority(RESET_WDG,
					  RESET_SOURCE_DEFAULT_PRIORITY);

	dev_info(dev, "reset reason: %s\n", reset_source_name());

	if (test_bit(F71808FG_FLAG_WD_EN, &wdt_conf))
		wdd->running = WDOG_HW_RUNNING;
	else
		wdd->running = WDOG_HW_NOT_RUNNING;

	ret = watchdog_register(wdd);
	if (ret)
		return ret;

	superio_enter(wd->sioaddr);
	dev_info(dev, "revision %d probed.\n",
		 superio_inb(wd->sioaddr, SIO_REG_DEVREV));
	superio_exit(wd->sioaddr);

	return 0;
}

static void f71808fg_pinconf(struct f71808e_wdt *wd)
{
	/* Set pin 21 to GPIO23/WDTRST#, then to WDTRST# */
	superio_clear_bit(wd->sioaddr, SIO_REG_MFUNCT2, 3);
	superio_clear_bit(wd->sioaddr, SIO_REG_MFUNCT3, 3);
}
static void f71862fg_pinconf(struct f71808e_wdt *wd)
{
	u16 ioaddr = wd->sioaddr;

	if (wd->f71862fg_pin == WDTRST_PIN_63) {
		/* SPI must be disabled first to use this pin! */
		superio_clear_bit(ioaddr, SIO_REG_ROM_ADDR_SEL, 6);
		superio_set_bit(ioaddr, SIO_REG_MFUNCT3, 4);
	} else if (wd->f71862fg_pin == WDTRST_PIN_56) {
		superio_set_bit(ioaddr, SIO_REG_MFUNCT1, 1);
	}
}
static void f71868_pinconf(struct f71808e_wdt *wd)
{
	/* GPIO14 --> WDTRST# */
	superio_clear_bit(wd->sioaddr, SIO_REG_MFUNCT1, 4);
}
static void f71882fg_pinconf(struct f71808e_wdt *wd)
{
	/* Set pin 56 to WDTRST# */
	superio_set_bit(wd->sioaddr, SIO_REG_MFUNCT1, 1);
}
static void f71889fg_pinconf(struct f71808e_wdt *wd)
{
	/* set pin 40 to WDTRST# */
	superio_outb(wd->sioaddr, SIO_REG_MFUNCT3,
		superio_inb(wd->sioaddr, SIO_REG_MFUNCT3) & 0xcf);
}
static void f81865_pinconf(struct f71808e_wdt *wd)
{
	/* Set pin 70 to WDTRST# */
	superio_clear_bit(wd->sioaddr, SIO_REG_MFUNCT3, 5);
}
static void f81866_pinconf(struct f71808e_wdt *wd)
{
	/*
	 * GPIO1 Control Register when 27h BIT3:2 = 01 & BIT0 = 0.
	 * The PIN 70(GPIO15/WDTRST) is controlled by 2Ch:
	 *     BIT5: 0 -> WDTRST#
	 *           1 -> GPIO15
	 */
	u8 tmp = superio_inb(wd->sioaddr, SIO_F81866_REG_PORT_SEL);
	tmp &= ~(BIT(3) | BIT(0));
	tmp |= BIT(2);
	superio_outb(wd->sioaddr, SIO_F81866_REG_PORT_SEL, tmp);

	superio_clear_bit(wd->sioaddr, SIO_F81866_REG_GPIO1, 5);
}

static struct f71808e_variant_data f71808fg_data = { .type = f71808fg, .pinconf = f71808fg_pinconf };
static struct f71808e_variant_data f71862fg_data = { .type = f71862fg, .pinconf = f71862fg_pinconf };
static struct f71808e_variant_data f71868_data = { .type = f71868, .pinconf = f71868_pinconf };
static struct f71808e_variant_data f71869_data = { .type = f71869, .pinconf = f71868_pinconf };
static struct f71808e_variant_data f71882fg_data = { .type = f71882fg, .pinconf = f71882fg_pinconf };
static struct f71808e_variant_data f71889fg_data = { .type = f71889fg, .pinconf = f71889fg_pinconf };
static struct f71808e_variant_data f81865_data = { .type = f81865, .pinconf = f81865_pinconf };
static struct f71808e_variant_data f81866_data = { .type = f81866, .pinconf = f81866_pinconf };

static struct platform_device_id f71808e_wdt_ids[] = {
	{ .name = "f71808fg_wdt", .driver_data = (unsigned long)&f71808fg_data },
	{ .name = "f71862fg_wdt", .driver_data = (unsigned long)&f71862fg_data },
	{ .name = "f71868_wdt", .driver_data = (unsigned long)&f71868_data },
	{ .name = "f71869_wdt", .driver_data = (unsigned long)&f71869_data },
	{ .name = "f71882fg_wdt", .driver_data = (unsigned long)&f71882fg_data },
	{ .name = "f71889fg_wdt", .driver_data = (unsigned long)&f71889fg_data },
	{ .name = "f81865_wdt", .driver_data = (unsigned long)&f81865_data },
	{ .name = "f81866_wdt", .driver_data = (unsigned long)&f81866_data },
	{ /* sentinel */ },
};

static int f71808e_probe(struct device_d *dev)
{
	struct f71808e_wdt *wd;
	struct resource *res;
	int ret;

	wd = xzalloc(sizeof(*wd));

	ret = dev_get_drvdata(dev, (const void **)&wd->variant);
	if (ret)
		return ret;

	res = dev_get_resource(dev->parent, IORESOURCE_IO, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);
	wd->sioaddr = res->start;

	return f71808e_wdt_init(wd, dev);
}

static struct driver_d f71808e_wdt_driver = {
	.probe	= f71808e_probe,
	.name	= "f71808e_wdt",
	.id_table = f71808e_wdt_ids,
};

device_platform_driver(f71808e_wdt_driver);
