// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2026 Sohaib Mohamed <sohaib.amhmd@gmail.com>
/*
 * Amlogic Meson UART driver, the baud rate setup and the error handling
 * are based on Linux' drivers/tty/serial/meson_uart.c.
 */

#include <console.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/math.h>
#include <serial/meson.h>

struct meson_uart_data {
	bool has_xtal_div2;
	const char *linux_console_name;
};

struct meson_uart {
	struct console_device cdev;
	void __iomem *base;
	unsigned long uartclk;
	const struct meson_uart_data *data;
};

static struct meson_uart *to_meson_uart(struct console_device *cdev)
{
	return container_of(cdev, struct meson_uart, cdev);
}

static void meson_uart_putc_cdev(struct console_device *cdev, char c)
{
	meson_uart_putc(to_meson_uart(cdev)->base, c);
}

static int meson_uart_tstc(struct console_device *cdev)
{
	struct meson_uart *meson = to_meson_uart(cdev);

	return !(readl(meson->base + AML_UART_STATUS) & AML_UART_RX_EMPTY);
}

static int meson_uart_getc(struct console_device *cdev)
{
	struct meson_uart *meson = to_meson_uart(cdev);
	u32 val;

	while (!meson_uart_tstc(cdev))
		;

	/* the error bits stay set until cleared, so do it by hand */
	if (readl(meson->base + AML_UART_STATUS) & AML_UART_ERR) {
		val = readl(meson->base + AML_UART_CONTROL);
		writel(val | AML_UART_CLEAR_ERR, meson->base + AML_UART_CONTROL);
		writel(val & ~AML_UART_CLEAR_ERR, meson->base + AML_UART_CONTROL);
	}

	return readl(meson->base + AML_UART_RFIFO) & 0xff;
}

static void meson_uart_flush(struct console_device *cdev)
{
	struct meson_uart *meson = to_meson_uart(cdev);

	while ((readl(meson->base + AML_UART_STATUS) &
		(AML_UART_TX_EMPTY | AML_UART_XMIT_BUSY)) != AML_UART_TX_EMPTY)
		;
}

static int meson_uart_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct meson_uart *meson = to_meson_uart(cdev);
	u32 val = 0;

	meson_uart_flush(cdev);

	if (meson->uartclk == 24000000) {
		unsigned int xtal_div = 3;

		if (meson->data->has_xtal_div2) {
			xtal_div = 2;
			val |= AML_UART_BAUD_XTAL_DIV2;
		}
		val |= DIV_ROUND_CLOSEST(meson->uartclk / xtal_div, baudrate) - 1;
		val |= AML_UART_BAUD_XTAL;
	} else {
		val = DIV_ROUND_CLOSEST(meson->uartclk / 4, baudrate) - 1;
	}
	val |= AML_UART_BAUD_USE;

	writel(val, meson->base + AML_UART_REG5);

	return 0;
}

static void meson_serial_init(struct meson_uart *meson)
{
	void __iomem *base = meson->base;
	u32 val;

	val = readl(base + AML_UART_CONTROL);
	val |= AML_UART_RX_RST | AML_UART_TX_RST | AML_UART_CLEAR_ERR;
	writel(val, base + AML_UART_CONTROL);
	val &= ~(AML_UART_RX_RST | AML_UART_TX_RST | AML_UART_CLEAR_ERR);

	/* 8N1 without flow control */
	val &= ~(AML_UART_DATA_LEN_MASK | AML_UART_PARITY_EN |
		 AML_UART_STOP_BIT_LEN_MASK);
	val |= AML_UART_DATA_LEN_8BIT | AML_UART_STOP_BIT_1SB |
	       AML_UART_TWO_WIRE_EN | AML_UART_TX_EN | AML_UART_RX_EN;
	writel(val, base + AML_UART_CONTROL);
}

static int meson_uart_probe(struct device *dev)
{
	struct meson_uart *meson;
	struct console_device *cdev;
	struct resource *iores;
	struct clk *clk;

	meson = xzalloc(sizeof(*meson));
	cdev = &meson->cdev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	meson->base = IOMEM(iores->start);
	meson->data = device_get_match_data(dev);

	clk = clk_get_enabled_for_console(dev, "pclk");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	clk = clk_get_enabled_for_console(dev, "xtal");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	clk = clk_get_enabled_for_console(dev, "baud");
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	meson->uartclk = clk_get_rate(clk);

	meson_serial_init(meson);

	cdev->dev = dev;
	cdev->tstc = meson_uart_tstc;
	cdev->putc = meson_uart_putc_cdev;
	cdev->getc = meson_uart_getc;
	cdev->flush = meson_uart_flush;
	/* without a clock rate, the baud rate the firmware set stays */
	if (meson->uartclk)
		cdev->setbrg = meson_uart_setbaudrate;
	cdev->linux_console_name = meson->data->linux_console_name ?: "ttyAML";
	cdev->linux_earlycon_name = "meson";
	cdev->phys_base = meson->base;

	return console_register(cdev);
}

static const struct meson_uart_data meson_gx_uart_data = {
	.has_xtal_div2 = false,
};

static const struct meson_uart_data meson_g12a_uart_data = {
	.has_xtal_div2 = true,
};

/* Linux registers these as ttyS, not ttyAML */
static const struct meson_uart_data meson_s4_uart_data = {
	.has_xtal_div2 = true,
	.linux_console_name = "ttyS",
};

static const struct of_device_id meson_uart_dt_ids[] = {
	{
		.compatible = "amlogic,meson-gx-uart",
		.data = &meson_gx_uart_data,
	},
	{
		.compatible = "amlogic,meson-g12a-uart",
		.data = &meson_g12a_uart_data,
	},
	{
		.compatible = "amlogic,meson-s4-uart",
		.data = &meson_s4_uart_data,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, meson_uart_dt_ids);

static struct driver meson_uart_driver = {
	.name = "meson_uart",
	.probe = meson_uart_probe,
	.of_compatible = DRV_OF_COMPAT(meson_uart_dt_ids),
};
console_platform_driver(meson_uart_driver);
