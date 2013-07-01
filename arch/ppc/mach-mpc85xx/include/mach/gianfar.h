/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2004, 2007, 2009  Freescale Semiconductor, Inc.
 * (C) Copyright 2003, Motorola, Inc.
 * based on tsec.h by Xianghua Xiao and Andy Fleming 2003-2009
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
 * Platform data for the Motorola Triple Speed Ethernet Controller
 */

#define GFAR_TBIPA_OFFSET       0x030   /* TBI PHY address */
#define GFAR_TBIPA_END		0x1f    /* Last valid PHY address */

struct gfar_info_struct {
	unsigned int phyaddr;
	unsigned int tbiana;
	unsigned int tbicr;
	unsigned int mdiobus_tbi;
};

int fsl_eth_init(int num, struct gfar_info_struct *gf);
