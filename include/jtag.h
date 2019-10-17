/*
 * include/linux/jtag.h
 *
 * Written Aug 2009 by Davide Rizzo <elpa.rizzo@gmail.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This driver manages one or more jtag chains controlled by host pins.
 * Jtag chains must be defined during setup using jtag_platdata structs.
 * All operations must be done from user programs using ioctls to /dev/jtag
 * Typical operation sequence is:
 * - open() the device (normally /dev/jtag)
 * - ioctl JTAG_GET_DEVICES reads how many devices in the chain
 * (repeat for each chip in the chain)
 * - ioctl JTAG_GET_ID identifies the chip
 * - ioctl JTAG_SET_IR_LENGTH sets the instruction register length
 * Before accessing the data registers, instruction registers' lengths
 *  MUST be programmed for all chips.
 * After this initialization, you can execute JTAG_IR_WR, JTAG_DR_RD, JTAG_DR_WR
 *  commands in any sequence.
 */

#ifndef __JTAG_H__
#define __JTAG_H__

/* Controller's gpio_tdi must be connected to last device's gpio_tdo */
/* Controller's gpio_tdo must be connected to first device's gpio_tdi */
struct jtag_platdata {
	unsigned int gpio_tclk;
	unsigned int gpio_tms;
	unsigned int gpio_tdi;
	unsigned int gpio_tdo;
	unsigned int gpio_trst;
	int use_gpio_trst;
};

#define JTAG_NAME "jtag"

/* structures used for passing arguments to ioctl */

struct jtag_rd_id {
	int device; /* Device in the chain */
	unsigned long id;
};

struct jtag_cmd {
	int device; /* Device in the chain (-1 = all devices) */
	unsigned int bitlen; /* Bit length of the register to be transfered */
	unsigned long *data; /* Data to be transfered */
};

/* Use 'j' as magic number */
#define JTAG_IOC_MAGIC		'j'

/* ioctl commands */

/* Resets jtag chain status, arg is ignored */
#define JTAG_RESET		_IO(JTAG_IOC_MAGIC, 0)

/* Returns the number of devices in the jtag chain, arg is ignored. */
#define JTAG_GET_DEVICES	_IO(JTAG_IOC_MAGIC, 1)

/* arg must point to a jtag_rd_id structure.
   Fills up the id field with ID of selected device */
#define JTAG_GET_ID		_IOR(JTAG_IOC_MAGIC, 2, struct jtag_rd_id)

/* arg must point to a struct jtag_cmd.
   Programs the Instruction Register length of specified device at bitlen value.
   *data is ignored. */
#define JTAG_SET_IR_LENGTH	_IOW(JTAG_IOC_MAGIC, 3, struct jtag_rd_id)

/* arg must point to a struct jtag_cmd.
   Writes *data in the Instruction Register of selected device, and BYPASS
    instruction into Instruction Registers of all other devices in the chain.
   If device == -1, the Instruction Registers of all devices are programmed
    to the same value.
   bitlen is always ignored, before using this command you have to program all
    Instruction Register's lengthes with JTAG_SET_IR_LENGTH command. */
#define JTAG_IR_WR		_IOW(JTAG_IOC_MAGIC, 4, struct jtag_cmd)

/* arg must point to a struct jtag_cmd.
   Reads data register of selected device, with length bitlen */
#define JTAG_DR_RD		_IOR(JTAG_IOC_MAGIC, 5, struct jtag_cmd)

/* arg must point to a struct jtag_cmd.
   Writes data register of selected device, with length bitlen.
   If device == -1, writes same data on all devices. */
#define JTAG_DR_WR		_IOW(JTAG_IOC_MAGIC, 6, struct jtag_cmd)

/* Generates arg pulses on TCLK pin */
#define JTAG_CLK		_IOW(JTAG_IOC_MAGIC, 7, unsigned int *)

#define JTAG_IOC_MAXNR		9

#endif /* __JTAG_H__ */
