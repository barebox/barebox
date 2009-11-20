/*
 * Copyright (C) 2008 Sascha Hauer, Pengutronix
 *
 * Derived from Linux SPI Framework
 *
 * Copyright (C) 2005 David Brownell
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <spi/spi.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>

/* SPI devices should normally not be created by SPI device drivers; that
 * would make them board-specific.  Similarly with SPI master drivers.
 * Device registration normally goes into like arch/.../mach.../board-YYY.c
 * with other readonly (flashable) information about mainboard devices.
 */

struct boardinfo {
	struct list_head	list;
	unsigned		n_board_info;
	struct spi_board_info	board_info[0];
};

static LIST_HEAD(board_list);

/**
 * spi_new_device - instantiate one new SPI device
 * @master: Controller to which device is connected
 * @chip: Describes the SPI device
 * Context: can sleep
 *
 * On typical mainboards, this is purely internal; and it's not needed
 * after board init creates the hard-wired devices.  Some development
 * platforms may not be able to use spi_register_board_info though, and
 * this is exported so that for example a USB or parport based adapter
 * driver could add devices (which it would learn about out-of-band).
 *
 * Returns the new device, or NULL.
 */
struct spi_device *spi_new_device(struct spi_master *master,
				  struct spi_board_info *chip)
{
	struct spi_device	*proxy;
	int			status;

	/* Chipselects are numbered 0..max; validate. */
	if (chip->chip_select >= master->num_chipselect) {
		debug("cs%d > max %d\n",
			chip->chip_select,
			master->num_chipselect);
		return NULL;
	}

	proxy = xzalloc(sizeof *proxy);
	proxy->master = master;
	proxy->chip_select = chip->chip_select;
	proxy->max_speed_hz = chip->max_speed_hz;
	proxy->mode = chip->mode;
	strcpy(proxy->dev.name, chip->name);
	proxy->dev.type_data = proxy;
	status = register_device(&proxy->dev);

	/* drivers may modify this initial i/o setup */
	status = master->setup(proxy);
	if (status < 0) {
		printf("can't setup %s, status %d\n",
				proxy->dev.name, status);
		goto fail;
	}

	return proxy;

fail:
	free(proxy);
	return NULL;
}
EXPORT_SYMBOL(spi_new_device);

/**
 * spi_register_board_info - register SPI devices for a given board
 * @info: array of chip descriptors
 * @n: how many descriptors are provided
 * Context: can sleep
 *
 * Board-specific early init code calls this (probably during arch_initcall)
 * with segments of the SPI device table.  Any device nodes are created later,
 * after the relevant parent SPI controller (bus_num) is defined.  We keep
 * this table of devices forever, so that reloading a controller driver will
 * not make Linux forget about these hard-wired devices.
 *
 * Other code can also call this, e.g. a particular add-on board might provide
 * SPI devices through its expansion connector, so code initializing that board
 * would naturally declare its SPI devices.
 *
 * The board info passed can safely be __initdata ... but be careful of
 * any embedded pointers (platform_data, etc), they're copied as-is.
 */
int
spi_register_board_info(struct spi_board_info const *info, int n)
{
	struct boardinfo	*bi;

	bi = xmalloc(sizeof(*bi) + n * sizeof *info);

	bi->n_board_info = n;
	memcpy(bi->board_info, info, n * sizeof *info);

	list_add_tail(&bi->list, &board_list);

	return 0;
}

static void scan_boardinfo(struct spi_master *master)
{
	struct boardinfo	*bi;

	list_for_each_entry(bi, &board_list, list) {
		struct spi_board_info	*chip = bi->board_info;
		unsigned		n;

		for (n = bi->n_board_info; n > 0; n--, chip++) {
			debug("%s %d %d\n", __FUNCTION__, chip->bus_num, master->bus_num);
			if (chip->bus_num != master->bus_num)
				continue;
			/* NOTE: this relies on spi_new_device to
			 * issue diagnostics when given bogus inputs
			 */
			(void) spi_new_device(master, chip);
		}
	}
}

/**
 * spi_register_master - register SPI master controller
 * @master: initialized master, originally from spi_alloc_master()
 * Context: can sleep
 *
 * SPI master controllers connect to their drivers using some non-SPI bus,
 * such as the platform bus.  The final stage of probe() in that code
 * includes calling spi_register_master() to hook up to this SPI bus glue.
 *
 * SPI controllers use board specific (often SOC specific) bus numbers,
 * and board-specific addressing for SPI devices combines those numbers
 * with chip select numbers.  Since SPI does not directly support dynamic
 * device identification, boards need configuration tables telling which
 * chip is at which address.
 *
 * This must be called from context that can sleep.  It returns zero on
 * success, else a negative error code (dropping the master's refcount).
 * After a successful return, the caller is responsible for calling
 * spi_unregister_master().
 */
int spi_register_master(struct spi_master *master)
{
	int			status = -ENODEV;

	debug("%s: %s:%d\n", __func__, master->dev->name, master->dev->id);

	/* even if it's just one always-selected device, there must
	 * be at least one chipselect
	 */
	if (master->num_chipselect == 0)
		return -EINVAL;

	/* populate children from any spi device tables */
	scan_boardinfo(master);
	status = 0;

	return status;
}
EXPORT_SYMBOL(spi_register_master);

int spi_sync(struct spi_device *spi, struct spi_message *message)
{
	return spi->master->transfer(spi, message);
}

