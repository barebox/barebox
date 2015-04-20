/*
 * SPI master driver using generic bitbanged GPIO
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux driver
 *   Copyright (C) 2006,2008 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
