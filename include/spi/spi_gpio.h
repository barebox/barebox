/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * SPI master driver using generic bitbanged GPIO
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux driver
 *   Copyright (C) 2006,2008 David Brownell
 */

#ifndef __SPI_GPIO_H
#define __SPI_GPIO_H

#define MAX_CHIPSELECT		4
#define SPI_GPIO_NO_CS		(-1)
#define SPI_GPIO_NO_MISO	(-1)
#define SPI_GPIO_NO_MOSI	(-1)

struct gpio_spi_pdata {
	int sck;
	int mosi;
	int miso;
	int cs[MAX_CHIPSELECT];
	int num_cs;
};

#endif
