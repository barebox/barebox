/*
 * Copyright 2013 GE Intelligent Platforms, Inc
 * Copyright 2006,2009 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Early I2C support functions to read SPD data or board
 * information.
 * Based on U-Boot drivers/i2c/fsl_i2c.c
 */
#include <common.h>
#include <i2c/i2c.h>
#include <mach/clock.h>
#include <mach/immap_85xx.h>
#include <mach/early_udelay.h>

/* FSL I2C registers */
#define FSL_I2C_ADR	0x00
#define FSL_I2C_FDR	0x04
#define FSL_I2C_CR	0x08
#define FSL_I2C_SR	0x0C
#define FSL_I2C_DR	0x10
#define FSL_I2C_DFSRR	0x14

/* Bits of FSL I2C registers */
#define I2C_SR_RXAK	0x01
#define I2C_SR_MIF	0x02
#define I2C_SR_MAL	0x10
#define I2C_SR_MBB	0x20
#define I2C_SR_MCF	0x80
#define I2C_CR_RSTA	0x04
#define I2C_CR_TXAK	0x08
#define I2C_CR_MTX	0x10
#define I2C_CR_MSTA	0x20
#define I2C_CR_MEN	0x80

/*
 * Set the I2C bus speed for a given I2C device
 *
 * parameters:
 * - i2c: the I2C base address.
 * - i2c_clk: I2C bus clock frequency.
 * - speed: the desired speed of the bus.
 *
 * The I2C device must be stopped before calling this function.
 *
 * The return value is the actual bus speed that is set.
 */
static uint32_t fsl_set_i2c_bus_speed(const void __iomem *i2c,
		uint32_t i2c_clk, uint32_t speed)
{
	uint8_t dfsr, fdr = 0x31; /* Default if no FDR found */
	uint16_t a, b, ga, gb, bin_gb, bin_ga, divider;
	uint32_t c_div, est_div;

	divider = min((uint16_t)(i2c_clk / speed), (uint16_t) -1);
	/* Condition 1: dfsr <= 50/T */
	dfsr = (5 * (i2c_clk / 1000)) / 100000;
	if (!dfsr)
		dfsr = 1;
	est_div = ~0;

	/*
	 * Bus speed is calculated as per Freescale AN2919.
	 * a, b and dfsr matches identifiers A,B and C as in the
	 * application note.
	 */
	for (ga = 0x4, a = 10; a <= 30; ga++, a += 2) {
		for (gb = 0; gb < 8; gb++) {
			b = 16 << gb;
			c_div = b * (a + (((3 * dfsr) / b) * 2));

			if ((c_div > divider) && (c_div < est_div)) {
				est_div = c_div;
				bin_gb = gb << 2;
				bin_ga = (ga & 0x3) | ((ga & 0x4) << 3);
				fdr = bin_gb | bin_ga;
				speed = i2c_clk / est_div;
			}
		}
		if (a == 20)
			a += 2;
		if (a == 24)
			a += 4;
	}
	writeb(dfsr, i2c + FSL_I2C_DFSRR);	/* set default filter */
	writeb(fdr, i2c + FSL_I2C_FDR);		/* set bus speed */

	return speed;
}

void fsl_i2c_init(void __iomem *i2c, int speed, int slaveadd)
{
	uint32_t i2c_clk;

	i2c_clk =  fsl_get_i2c_freq();
	writeb(0, i2c + FSL_I2C_CR);
	early_udelay(5);

	fsl_set_i2c_bus_speed(i2c, i2c_clk, speed);
	writeb(slaveadd << 1, i2c + FSL_I2C_ADR);
	writeb(0x0, i2c + FSL_I2C_SR);
	writeb(I2C_CR_MEN, i2c + FSL_I2C_CR);
}

static uint32_t fsl_usec2ticks(uint32_t usec)
{
	ulong ticks;

	if (usec < 1000) {
		ticks = (usec * (fsl_get_timebase_clock() / 1000));
		ticks = (ticks + 500) / 1000;
	} else {
		ticks = (usec / 10);
		ticks *= (fsl_get_timebase_clock() / 100000);
	}

	return ticks;
}

static int fsl_i2c_wait4bus(void __iomem *i2c)
{
	uint64_t timeval = get_ticks();
	const uint64_t timeout = fsl_usec2ticks(20000);

	while (readb(i2c + FSL_I2C_SR) & I2C_SR_MBB)
		if ((get_ticks() - timeval) > timeout)
			return -1;

	return 0;
}

void fsl_i2c_stop(void __iomem *i2c)
{
	writeb(I2C_CR_MEN, i2c + FSL_I2C_CR);
}

static int fsl_i2c_wait(void __iomem *i2c, int write)
{
	const uint64_t timeout = fsl_usec2ticks(100000);
	uint64_t timeval = get_ticks();
	int csr;

	do {
		csr = readb(i2c + FSL_I2C_SR);
		if (csr & I2C_SR_MIF)
			break;
	} while ((get_ticks() - timeval) < timeout);

	if ((get_ticks() - timeval) > timeout)
		goto error;

	csr = readb(i2c + FSL_I2C_SR);
	writeb(0x0, i2c + FSL_I2C_SR);

	if (csr & I2C_SR_MAL)
		goto error;

	if (!(csr & I2C_SR_MCF))
		goto error;

	if (write == 0 && (csr & I2C_SR_RXAK))
		goto error;

	return 0;
error:
	return -1;
}

static int __i2c_write(void __iomem *i2c, uint8_t *data, int length)
{
	int i;

	for (i = 0; i < length; i++) {
		writeb(data[i], i2c + FSL_I2C_DR);
		if (fsl_i2c_wait(i2c, 0) < 0)
			break;
	}

	return i;
}

static int __i2c_read(void __iomem *i2c, uint8_t *data, int length)
{
	int i;
	uint8_t val = I2C_CR_MEN | I2C_CR_MSTA;

	if (length == 1)
		writeb(val | I2C_CR_TXAK, i2c + FSL_I2C_CR);
	else
		writeb(val, i2c + FSL_I2C_CR);

	readb(i2c + FSL_I2C_DR);
	for (i = 0; i < length; i++) {
		if (fsl_i2c_wait(i2c, 1) < 0)
			break;

		/* Generate ack on last next to last byte */
		if (i == length - 2)
			writeb(val | I2C_CR_TXAK, i2c + FSL_I2C_CR);
		/* Do not generate stop on last byte */
		if (i == length - 1)
			writeb(val | I2C_CR_MTX, i2c + FSL_I2C_CR);

		data[i] = readb(i2c + FSL_I2C_DR);
	}

	return i;
}

static int
fsl_i2c_write_addr(void __iomem *i2c, uint8_t dev, uint8_t dir, int rsta)
{
	uint8_t val = I2C_CR_MEN | I2C_CR_MSTA | I2C_CR_MTX;

	if (rsta)
		val |= I2C_CR_RSTA;
	writeb(val, i2c + FSL_I2C_CR);
	writeb((dev << 1) | dir, i2c + FSL_I2C_DR);

	if (fsl_i2c_wait(i2c, 0) < 0)
		return 0;

	return 1;
}

int fsl_i2c_read(void __iomem *i2c, uint8_t dev, uint addr, int alen,
			uint8_t *data, int length)
{
	int i = -1;
	uint8_t *a = (uint8_t *)&addr;

	if (alen && (fsl_i2c_wait4bus(i2c) >= 0) &&
		(fsl_i2c_write_addr(i2c, dev, 0, 0) != 0) &&
		(__i2c_write(i2c, &a[4 - alen], alen) == alen))
		i = 0;

	if (length && fsl_i2c_write_addr(i2c, dev, 1, 1) != 0)
		i = __i2c_read(i2c, data, length);

	if (i == length)
		return 0;

	return -1;
}
