// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Anup Patel <anup@brainfault.org>
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <io.h>
#include <of.h>
#include <linux/err.h>
#include <linux/clk.h>

#define UART_TXFIFO_FULL	0x80000000
#define UART_RXFIFO_EMPTY	0x80000000
#define UART_RXFIFO_DATA	0x000000ff
#define UART_TXCTRL_TXEN	0x1
#define UART_RXCTRL_RXEN	0x1

/* IP register */
#define UART_IP_RXWM            0x2

struct sifive_serial_regs {
	u32 txfifo;
	u32 rxfifo;
	u32 txctrl;
	u32 rxctrl;
	u32 ie;
	u32 ip;
	u32 div;
};

struct sifive_serial_priv {
	unsigned long freq;
	struct sifive_serial_regs __iomem *regs;
	struct console_device cdev;
};

#define to_priv(cdev) container_of(cdev, struct sifive_serial_priv, cdev)

/**
 * Find minimum divisor divides in_freq to max_target_hz;
 * Based on uart driver n SiFive FSBL.
 *
 * f_baud = f_in / (div + 1) => div = (f_in / f_baud) - 1
 * The nearest integer solution requires rounding up as to not exceed
 * max_target_hz.
 * div  = ceil(f_in / f_baud) - 1
 *	= floor((f_in - 1 + f_baud) / f_baud) - 1
 * This should not overflow as long as (f_in - 1 + f_baud) does not exceed
 * 2^32 - 1, which is unlikely since we represent frequencies in kHz.
 */
static inline unsigned int uart_min_clk_divisor(unsigned long in_freq,
						unsigned long max_target_hz)
{
	unsigned long quotient =
			(in_freq + max_target_hz - 1) / (max_target_hz);
	/* Avoid underflow */
	if (quotient == 0)
		return 0;
	else
		return quotient - 1;
}

static void sifive_serial_init(struct sifive_serial_regs __iomem *regs)
{
	writel(UART_TXCTRL_TXEN, &regs->txctrl);
	writel(UART_RXCTRL_RXEN, &regs->rxctrl);
	writel(0, &regs->ie);
}

static int sifive_serial_setbrg(struct console_device *cdev, int baudrate)
{
	struct sifive_serial_priv *priv = to_priv(cdev);

	writel((uart_min_clk_divisor(priv->freq, baudrate)), &priv->regs->div);

	return 0;
}

static int sifive_serial_getc(struct console_device *cdev)
{
	struct sifive_serial_regs __iomem *regs = to_priv(cdev)->regs;
	u32 ch;

	do {
		ch = readl(&regs->rxfifo);
	} while (ch & UART_RXFIFO_EMPTY);

	return ch & UART_RXFIFO_DATA;
}

static void sifive_serial_putc(struct console_device *cdev, const char ch)
{
	struct sifive_serial_regs __iomem *regs = to_priv(cdev)->regs;

	// TODO: how to check for !empty to utilize fifo?
	while (readl(&regs->txfifo) & UART_TXFIFO_FULL)
		;

	writel(ch, &regs->txfifo);
}

static int sifive_serial_tstc(struct console_device *cdev)
{
	struct sifive_serial_regs __iomem *regs = to_priv(cdev)->regs;

	return readl(&regs->ip) & UART_IP_RXWM;
}

static void sifive_serial_flush(struct console_device *cdev)
{
	struct sifive_serial_regs __iomem *regs = to_priv(cdev)->regs;

	while (readl(&regs->txfifo) & UART_TXFIFO_FULL)
		;
}

static int sifive_serial_probe(struct device *dev)
{
	struct sifive_serial_priv *priv;
	struct resource *iores;
	struct clk *clk;
	u32 freq;
	int ret;

	clk = clk_get(dev, NULL);
	if (!IS_ERR(clk)) {
		freq = clk_get_rate(clk);
	} else {
		dev_dbg(dev, "failed to get clock. Fallback to device tree.\n");

		ret = of_property_read_u32(dev->of_node, "clock-frequency",
					   &freq);
		if (ret) {
			dev_warn(dev, "unknown clock frequency\n");
			return ret;
		}
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv = xzalloc(sizeof(*priv));

	priv->freq = freq;
	priv->regs = IOMEM(iores->start);

	priv->cdev.dev = dev;
	priv->cdev.putc = sifive_serial_putc;
	priv->cdev.getc = sifive_serial_getc;
	priv->cdev.tstc = sifive_serial_tstc;
	priv->cdev.flush = sifive_serial_flush;
	priv->cdev.setbrg = sifive_serial_setbrg,

	sifive_serial_init(priv->regs);

	return console_register(&priv->cdev);
}

static __maybe_unused struct of_device_id sifive_serial_dt_ids[] = {
	{ .compatible = "sifive,uart0" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sifive_serial_dt_ids);

static struct driver serial_sifive_driver = {
	.name   = "serial_sifive",
	.probe  = sifive_serial_probe,
	.of_compatible = sifive_serial_dt_ids,
};
console_platform_driver(serial_sifive_driver);
