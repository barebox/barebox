/*
 *	w1.h
 *
 * Copyright (c) 2004 Evgeniy Polyakov <zbr@ioremap.net>
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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

#ifndef __W1_H
#define __W1_H

#include <common.h>
#include <driver.h>

struct w1_device {
	u64	reg_num;
	u8	fid;
	u64	id;
	u8	crc;

	struct w1_bus	*bus;
	struct device_d dev;
};

struct w1_driver {
	u8	fid;
	u64	id;

	int	(*probe) (struct w1_device *dev);
	void	(*remove) (struct w1_device *dev);
	struct driver_d drv;
};

int w1_driver_register(struct w1_driver *drv);

extern struct bus_type w1_bustype;

#define W1_MAXNAMELEN		32

#define W1_SEARCH		0xF0
#define W1_ALARM_SEARCH		0xEC
#define W1_CONVERT_TEMP		0x44
#define W1_SKIP_ROM		0xCC
#define W1_READ_SCRATCHPAD	0xBE
#define W1_READ_ROM		0x33
#define W1_READ_PSUPPLY		0xB4
#define W1_MATCH_ROM		0x55
#define W1_RESUME_CMD		0xA5

#define W1_SLAVE_ACTIVE		0

/**
 * Note: read_bit and write_bit are very low level functions and should only
 * be used with hardware that doesn't really support 1-wire operations,
 * like a parallel/serial port.
 * Either define read_bit and write_bit OR define, at minimum, touch_bit and
 * reset_bus.
 */
struct w1_bus
{
	struct device_d	dev;
	struct device_d	*parent;

	/**
	 * Sample the line level
	 * @return the level read (0 or 1)
	 */
	u8		(*read_bit)(struct w1_bus *);

	/** Sets the line level */
	void		(*write_bit)(struct w1_bus *, u8);

	/**
	 * touch_bit is the lowest-level function for devices that really
	 * support the 1-wire protocol.
	 * touch_bit(0) = write-0 cycle
	 * touch_bit(1) = write-1 / read cycle
	 * @return the bit read (0 or 1)
	 */
	u8		(*touch_bit)(struct w1_bus *, u8);

	/**
	 * Reads a bytes. Same as 8 touch_bit(1) calls.
	 * @return the byte read
	 */
	u8		(*read_byte)(struct w1_bus *);

	/**
	 * Writes a byte. Same as 8 touch_bit(x) calls.
	 */
	void		(*write_byte)(struct w1_bus *, u8);

	/**
	 * Same as a series of read_byte() calls
	 * @return the number of bytes read
	 */
	u8		(*read_block)(struct w1_bus *, u8 *, int);

	/** Same as a series of write_byte() calls */
	void		(*write_block)(struct w1_bus *, const u8 *, int);

	/**
	 * Combines two reads and a smart write for ROM searches
	 * @return bit0=Id bit1=comp_id bit2=dir_taken
	 */
	u8		(*triplet)(struct w1_bus *, u8);

	/**
	 * long write-0 with a read for the presence pulse detection
	 * @return -1=Error, 0=Device present, 1=No device present
	 */
	u8		(*reset_bus)(struct w1_bus *);

	/**
	 * Put out a strong pull-up pulse of the specified duration.
	 * @return -1=Error, 0=completed
	 */
	u8		(*set_pullup)(struct w1_bus *, int);

	/** 5V strong pullup enabled flag, 1 enabled, zero disabled. */
	int		enable_pullup;
	/** 5V strong pullup duration in milliseconds, zero disabled. */
	int		pullup_duration;

	int		max_slave_count, slave_count;

	bool		is_searched;

	void		*data;
	struct list_head list;
};

u8 w1_read_block(struct w1_bus *bus, u8 *buf, int len);
void w1_write_block(struct w1_bus *bus, const u8 *buf, int len);
void w1_touch_block(struct w1_bus *bus, u8 *buf, int len);
u8 w1_triplet(struct w1_bus *bus, int bdir);
u8 w1_read_8(struct w1_bus *bus);
void w1_write_8(struct w1_bus *bus, u8 byte);
int w1_reset_select_slave(struct w1_device *dev);
int w1_reset_bus(struct w1_bus *bus);
u8 w1_calc_crc8(u8 * data, int len);

int w1_bus_register(struct w1_bus *bus);

#endif /* __W1_H */
