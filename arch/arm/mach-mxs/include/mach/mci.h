/*
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

#ifndef __MACH_MMC_H
#define __MACH_MMC_H

struct mxs_mci_platform_data {
	unsigned caps;	/**< supported operating modes (MMC_MODE_*) */
	unsigned voltages; /**< supported voltage range (MMC_VDD_*) */
	unsigned f_min;	/**< min operating frequency in Hz (0 -> no limit) */
	unsigned f_max;	/**< max operating frequency in Hz (0 -> no limit) */
	/* TODO */
	/* function to modify the voltage */
	/* function to switch the voltage */
	/* function to detect the presence of a SD card in the socket */
};

#endif /* __MACH_MMC_H */
