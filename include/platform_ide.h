/*
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#ifndef __PLATFORM_IDE_H
#define __PLATFORM_IDE_H

struct ide_port_info {
	/*
	 * I/O port shift, for platforms with ports that are
	 * constantly spaced and need larger than the 1-byte
	 * spacing
	 */
	unsigned ioport_shift;
	int dataif_be;	/* true if 16 bit data register is big endian */

	/* handle hard reset of this port */
	void (*reset)(int);	/* true: assert reset, false: de-assert reset */
};

#endif /* __PLATFORM_IDE_H */
