/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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

/**
 * @file
 * @brief Declarations to communicate with ATA types of drives
 */

#include <types.h>
#include <driver.h>

/**
 * Access functions from drives through the specified interface
 */
struct ata_interface {
	/** write a count of sectors from a buffer to the drive */
	int (*write)(struct device_d*, uint64_t, unsigned, const void*);
	/** read a count of sectors from the drive into the buffer */
	int (*read)(struct device_d*, uint64_t, unsigned, void*);
	/** private interface data */
	void *priv;
};
