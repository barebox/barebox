/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __MACH_MMC_H
#define __MACH_MMC_H

struct mxs_mci_platform_data {
	const char *devname;
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
