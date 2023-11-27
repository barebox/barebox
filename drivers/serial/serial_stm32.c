// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2016, STMicroelectronics - All Rights Reserved
 * Author(s): Vikas Manocha, <vikas.manocha@st.com> for STMicroelectronics.
 */

#include <common.h>
#include <console.h>
#include <io.h>
#include <linux/clk.h>
#include <init.h>
#include <driver.h>
#include "serial_stm32.h"

struct stm32_uart_info {
	u8 uart_enable_bit;	/* UART_CR1_UE */
	bool stm32f4;		/* true for STM32F4, false otherwise */
	bool has_fifo;
};

struct stm32_uart {
	struct console_device cdev;
	void __iomem *base;
	struct clk *clk;
	bool stm32f4;
	bool has_fifo;
	int uart_enable_bit;
};

static struct stm32_uart *to_stm32_uart(struct console_device *cdev)
{
	return container_of(cdev, struct stm32_uart, cdev);
}

static int stm32_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct stm32_uart *stm32 = to_stm32_uart(cdev);
	void __iomem *base = stm32->base;
	bool stm32f4 = stm32->stm32f4;
	u32 int_div, mantissa, fraction, oversampling;
	unsigned long clock_rate;

	clock_rate = clk_get_rate(stm32->clk);
	if (!clock_rate)
		return -EINVAL;

	int_div = DIV_ROUND_CLOSEST(clock_rate, baudrate);

	if (int_div < 16) {
		oversampling = 8;
		setbits_le32(base + CR1_OFFSET(stm32f4), USART_CR1_OVER8);
	} else {
		oversampling = 16;
		clrbits_le32(base + CR1_OFFSET(stm32f4), USART_CR1_OVER8);
	}

	mantissa = (int_div / oversampling) << USART_BRR_M_SHIFT;
	fraction = int_div % oversampling;

	writel(mantissa | fraction, base + BRR_OFFSET(stm32f4));

	return 0;
}

static int stm32_serial_getc(struct console_device *cdev)
{
	struct stm32_uart *stm32 = to_stm32_uart(cdev);
	void __iomem *base = stm32->base;
	bool stm32f4 = stm32->stm32f4;
	u32 isr = readl(base + ISR_OFFSET(stm32f4));

	if ((isr & USART_ISR_RXNE) == 0)
		return -EAGAIN;

	if (isr & (USART_ISR_PE | USART_ISR_ORE)) {
		if (!stm32f4)
			setbits_le32(base + ICR_OFFSET,
				     USART_ICR_PCECF | USART_ICR_ORECF);
		else
			readl(base + RDR_OFFSET(stm32f4));
		return -EIO;
	}

	return readl(base + RDR_OFFSET(stm32f4));
}

static void stm32_serial_putc(struct console_device *cdev, char c)
{
	struct stm32_uart *stm32 = to_stm32_uart(cdev);
	void __iomem *base = stm32->base;
	bool stm32f4 = stm32->stm32f4;

	while ((readl(base + ISR_OFFSET(stm32f4)) & USART_ISR_TXE) == 0);

	writel(c, base + TDR_OFFSET(stm32f4));
}

static int stm32_serial_tstc(struct console_device *cdev)
{
	struct stm32_uart *stm32 = to_stm32_uart(cdev);
	void __iomem *base = stm32->base;
	bool stm32f4 = stm32->stm32f4;

	return readl(base + ISR_OFFSET(stm32f4)) & USART_ISR_RXNE ? 1 : 0;
}

static void stm32_serial_flush(struct console_device *cdev)
{
	struct stm32_uart *stm32 = to_stm32_uart(cdev);
	void __iomem *base = stm32->base;
	bool stm32f4 = stm32->stm32f4;

	while (!(readl(base + ISR_OFFSET(stm32f4)) & USART_ISR_TXE));
}

static void stm32_serial_init(struct console_device *cdev)
{
	struct stm32_uart *stm32 = to_stm32_uart(cdev);
	void __iomem *base = stm32->base;
	bool stm32f4 = stm32->stm32f4;
	u8 uart_enable_bit = stm32->uart_enable_bit;
	u32 cr1;

	cr1 = readl(base + CR1_OFFSET(stm32f4));

	/* Disable uart-> enable fifo -> enable uart */
	cr1 &= ~(USART_CR1_RE | USART_CR1_TE | BIT(uart_enable_bit));

	writel(cr1, base + CR1_OFFSET(stm32f4));

	if (stm32->has_fifo)
		cr1 |= USART_CR1_FIFOEN;

	cr1 &= ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M1 | USART_CR1_M0);

	cr1 |= USART_CR1_RE | USART_CR1_TE | BIT(uart_enable_bit);

	writel(cr1, base + CR1_OFFSET(stm32f4));
}

static int stm32_serial_probe(struct device *dev)
{
	int ret;
	struct console_device *cdev;
	struct stm32_uart *stm32;
	const char *devname;
	struct resource *res;
	struct stm32_uart_info *info;

	ret = dev_get_drvdata(dev, (const void **)&info);
	if (ret)
		return ret;

	stm32 = xzalloc(sizeof(*stm32));
	cdev = &stm32->cdev;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res)) {
		ret = PTR_ERR(res);
		goto err_free;
	}
	stm32->base = IOMEM(res->start);

	stm32->has_fifo = info->has_fifo;
	stm32->stm32f4 = info->stm32f4;
	stm32->uart_enable_bit = info->uart_enable_bit;

	stm32->clk = clk_get_for_console(dev, NULL);
	if (IS_ERR(stm32->clk)) {
		ret = PTR_ERR(stm32->clk);
		dev_err(dev, "Failed to get UART clock %d\n", ret);
		goto io_release;
	}

	ret = clk_enable(stm32->clk);
	if (ret) {
		dev_err(dev, "Failed to enable UART clock %d\n", ret);
		goto io_release;
	}

	cdev->dev    = dev;
	cdev->tstc   = stm32_serial_tstc;
	cdev->putc   = stm32_serial_putc;
	cdev->getc   = stm32_serial_getc;
	cdev->flush  = stm32_serial_flush;
	cdev->setbrg = stm32->clk ? stm32_serial_setbaudrate : NULL;
	cdev->linux_console_name = "ttySTM";

	if (dev->of_node) {
		devname = of_alias_get(dev->of_node);
		if (devname) {
			cdev->devname = xstrdup(devname);
			cdev->devid   = DEVICE_ID_SINGLE;
		}
	}

	stm32_serial_init(cdev);

	ret = console_register(cdev);
	if (!ret)
		return 0;

	clk_put(stm32->clk);
io_release:
	release_region(res);
err_free:
	free(stm32);

	return ret;
}

struct stm32_uart_info stm32f4_info = {
	.stm32f4 = true,
	.uart_enable_bit = 13,
	.has_fifo = false,
};

struct stm32_uart_info stm32f7_info = {
	.uart_enable_bit = 0,
	.stm32f4 = false,
	.has_fifo = true,
};

struct stm32_uart_info stm32h7_info = {
	.uart_enable_bit = 0,
	.stm32f4 = false,
	.has_fifo = true,
};

static struct of_device_id stm32_serial_dt_ids[] = {
	{
		.compatible = "st,stm32-uart",
		.data = &stm32f4_info
	}, {
		.compatible = "st,stm32f7-uart",
		.data = &stm32f7_info
	}, {
		.compatible = "st,stm32h7-uart",
		.data = &stm32h7_info
	}, {
	}
};
MODULE_DEVICE_TABLE(of, stm32_serial_dt_ids);

static struct driver stm32_serial_driver = {
	.name   = "stm32-serial",
	.probe  = stm32_serial_probe,
	.of_compatible = DRV_OF_COMPAT(stm32_serial_dt_ids),
};
console_platform_driver(stm32_serial_driver);
