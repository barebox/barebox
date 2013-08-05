/**
 * @file
 * @brief Serial NS16550 platform specific header
 *
 * FileName: include/ns16550.h
 * @code struct NS16550_plat @endcode
 * represents The specifics of the device present in the system.
 */
/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
 */

#ifndef __NS16650_PLATFORM_H_
#define __NS16650_PLATFORM_H_

/**
 * @brief Platform dependent feature:
 * Pass pointer to this structure as part of device_d -> platform_data
 */
struct NS16550_plat {
	/** Clock speed */
	unsigned int clock;
	/**
	 * register read access capability
	 */
	unsigned int (*reg_read) (unsigned long base, unsigned char reg_offset);
	/**
	 * register write access capability
	 */
	void (*reg_write) (unsigned int val, unsigned long base,
				    unsigned char reg_offset);

	int shift;
	unsigned int flags;
#define NS16650_FLAG_DISABLE_FIFO	1
};

#endif				/* __NS16650_PLATFORM_H_ */
