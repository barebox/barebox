// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Marcelo Politzer <marcelo.politzer@cartesi.io>
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <asm/sbi.h>

struct sbi_serial_priv {
	struct console_device cdev;
	uint8_t b[2], head, tail;
};

static int sbi_serial_getc(struct console_device *cdev)
{
	struct sbi_serial_priv *priv = cdev->dev->priv;
	if (priv->head == priv->tail)
		return -1;
	return priv->b[priv->head++ & 0x1];
}

static void sbi_serial_putc(struct console_device *cdev, const char ch)
{
	sbi_console_putchar(ch);
}

static int sbi_serial_tstc(struct console_device *cdev)
{
	struct sbi_serial_priv *priv = cdev->dev->priv;
	int c = sbi_console_getchar();

	if (c != -1)
		priv->b[priv->tail++ & 0x1] = c;
	return priv->head != priv->tail;
}

static int sbi_serial_probe(struct device_d *dev)
{
	struct sbi_serial_priv *priv;

	priv = dev->priv = xzalloc(sizeof(*priv));
	priv->cdev.dev    = dev;
	priv->cdev.putc   = sbi_serial_putc;
	priv->cdev.getc   = sbi_serial_getc;
	priv->cdev.tstc   = sbi_serial_tstc;
	priv->cdev.flush  = 0;
	priv->cdev.setbrg = 0;

	return console_register(&priv->cdev);
}

static struct driver_d serial_sbi_driver = {
	.name   = "riscv-serial-sbi",
	.probe  = sbi_serial_probe,
};
postcore_platform_driver(serial_sbi_driver);
