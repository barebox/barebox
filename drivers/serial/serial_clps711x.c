// SPDX-License-Identifier: GPL-2.0-or-later
/* Author: Alexander Shiyan <shc_work@mail.ru> */

#include <common.h>
#include <malloc.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mfd/syscon.h>

#define UARTDR			0x00
# define UARTDR_FRMERR		(1 << 8)
# define UARTDR_PARERR		(1 << 9)
# define UARTDR_OVERR		(1 << 10)
#define UBRLCR			0x40
# define UBRLCR_BAUD_MASK	((1 << 12) - 1)
# define UBRLCR_BREAK		(1 << 12)
# define UBRLCR_PRTEN		(1 << 13)
# define UBRLCR_EVENPRT		(1 << 14)
# define UBRLCR_XSTOP		(1 << 15)
# define UBRLCR_FIFOEN		(1 << 16)
# define UBRLCR_WRDLEN5		(0 << 17)
# define UBRLCR_WRDLEN6		(1 << 17)
# define UBRLCR_WRDLEN7		(2 << 17)
# define UBRLCR_WRDLEN8		(3 << 17)
# define UBRLCR_WRDLEN_MASK	(3 << 17)

#define SYSCON			0x00
# define SYSCON_UARTEN		(1 << 8)
#define SYSFLG			0x40
# define SYSFLG_UBUSY		(1 << 11)
# define SYSFLG_URXFE		(1 << 22)
# define SYSFLG_UTXFF		(1 << 23)

struct clps711x_uart {
	void __iomem		*base;
	struct regmap		*regmap;
	struct clk		*uart_clk;
	struct console_device	cdev;
};

static int clps711x_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct clps711x_uart *s = cdev->dev->priv;
	int divisor;
	u32 tmp;

	divisor = DIV_ROUND_CLOSEST(clk_get_rate(s->uart_clk), baudrate * 16);

	tmp = readl(s->base + UBRLCR) & ~UBRLCR_BAUD_MASK;
	tmp |= divisor - 1;
	writel(tmp, s->base + UBRLCR);

	return 0;
}

static void clps711x_init_port(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u32 tmp;

	/* Disable the UART */
	regmap_update_bits(s->regmap, SYSCON, SYSCON_UARTEN, 0);

	/* Setup Line Control Register */
	tmp = readl(s->base + UBRLCR) & UBRLCR_BAUD_MASK;
	tmp |= UBRLCR_FIFOEN | UBRLCR_WRDLEN8; /* FIFO on, 8N1 mode */
	writel(tmp, s->base + UBRLCR);

	/* Enable the UART */
	regmap_update_bits(s->regmap, SYSCON, SYSCON_UARTEN, SYSCON_UARTEN);
}

static void clps711x_putc(struct console_device *cdev, char c)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u32 tmp;

	/* Wait until there is space in the FIFO */
	do {
		regmap_read(s->regmap, SYSFLG, &tmp);

	} while (tmp & SYSFLG_UTXFF);

	/* Send the character */
	writew(c, s->base + UARTDR);
}

static int clps711x_getc(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u16 data;
	u32 tmp;

	/* Wait until there is data in the FIFO */
	do {
		regmap_read(s->regmap, SYSFLG, &tmp);
	} while (tmp & SYSFLG_URXFE);

	data = readw(s->base + UARTDR);

	/* Check for an error flag */
	if (data & (UARTDR_FRMERR | UARTDR_PARERR | UARTDR_OVERR))
		return -1;

	return (int)data;
}

static int clps711x_tstc(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u32 tmp;

	regmap_read(s->regmap, SYSFLG, &tmp);

	return !(tmp & SYSFLG_URXFE);
}

static void clps711x_flush(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u32 tmp;

	do {
		regmap_read(s->regmap, SYSFLG, &tmp);
	} while (tmp & SYSFLG_UBUSY);
}

static int clps711x_probe(struct device *dev)
{
	struct device_node *syscon;
	struct clps711x_uart *s;
	const char *devname;
	int err;

	s = xzalloc(sizeof(struct clps711x_uart));

	s->uart_clk = clk_get(dev, NULL);
	if (IS_ERR(s->uart_clk)) {
		err = PTR_ERR(s->uart_clk);
		goto out_err;
	}

	s->base = dev_get_mem_region(dev, 0);
	if (IS_ERR(s->base)) {
		err = PTR_ERR(s->base);
		goto out_err;
	}

	syscon = of_parse_phandle(dev->of_node, "syscon", 0);
	s->regmap = syscon_node_to_regmap(syscon);
	if (IS_ERR(s->regmap)) {
		err = PTR_ERR(s->regmap);
		goto out_err;
	}

	dev->priv	= s;
	s->cdev.dev	= dev;
	s->cdev.tstc	= clps711x_tstc;
	s->cdev.putc	= clps711x_putc;
	s->cdev.getc	= clps711x_getc;
	s->cdev.flush	= clps711x_flush;
	s->cdev.setbrg	= clps711x_setbaudrate;
	s->cdev.linux_console_name = "ttyCL";

	devname = of_alias_get(dev->of_node);
	if (devname) {
		s->cdev.devname = xstrdup(devname);
		s->cdev.devid = DEVICE_ID_SINGLE;
	}

	clps711x_init_port(&s->cdev);

	err = console_register(&s->cdev);

out_err:
	if (err)
		free(s);

	return err;
}

static const struct of_device_id __maybe_unused clps711x_uart_dt_ids[] = {
	{ .compatible = "cirrus,ep7209-uart", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, clps711x_uart_dt_ids);

static struct driver clps711x_driver = {
	.name		= "clps711x-uart",
	.probe		= clps711x_probe,
	.of_compatible	= DRV_OF_COMPAT(clps711x_uart_dt_ids),
};
console_platform_driver(clps711x_driver);
