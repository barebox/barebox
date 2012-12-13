/*
 * CPSW Ethernet Switch Driver
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _CPSW_H_
#define _CPSW_H_

struct cpsw_slave_data {
	int		phy_id;
	int		phy_if;
};

struct cpsw_platform_data {
	struct cpsw_slave_data	*slave_data;
	int			num_slaves;
};

#endif /* _CPSW_H_  */
