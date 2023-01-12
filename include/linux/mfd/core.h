/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2006 Ian Molton
 * Copyright (c) 2007 Dmitry Baryshkov
 */

#ifndef MFD_CORE_H
#define MFD_CORE_H

struct device;

/*
 * This struct describes the MFD part ("cell").
 * After registration the copy of this structure will become the platform data
 * of the resulting device_d
 */
struct mfd_cell {
	const char		*name;
};

int mfd_add_devices(struct device *parent, const struct mfd_cell *cells,
		    int n_devs);

#endif
