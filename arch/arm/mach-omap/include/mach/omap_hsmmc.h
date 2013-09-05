/**
 * @file
 * @brief This file contains exported structure for OMAP hsmmc
 *
 * OMAP3 and OMAP4 has a MMC/SD controller embedded.
 * This file provides the platform data structure required to
 * addapt to platform specialities.
 *
 * (C) Copyright 2011
 * Phytec Messtechnik GmbH, <www.phytec.de>
 * Juergen Kilb <j.kilb@phytec.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_OMAP_HSMMC_H
#define  __ASM_OMAP_HSMMC_H

/** omapmmc platform data structure */
struct omap_hsmmc_platform_data {
	unsigned f_max;         /* host interface upper limit */
	char *devname;		/* The mci device name, optional */
};
#endif /* __ASM_OMAP_HSMMC_H */
