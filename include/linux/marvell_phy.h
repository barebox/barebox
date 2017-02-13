/*
 * Marvell PHY IDs
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

#ifndef _MARVELL_PHY_H
#define _MARVELL_PHY_H

/* Known PHY IDs */
#define MARVELL_PHY_ID_88E1101		0x01410c60
#define MARVELL_PHY_ID_88E1112		0x01410c90
#define MARVELL_PHY_ID_88E1111		0x01410cc0
#define MARVELL_PHY_ID_88E1118		0x01410e10
#define MARVELL_PHY_ID_88E1121R		0x01410cb0
#define MARVELL_PHY_ID_88E1145		0x01410cd0
#define MARVELL_PHY_ID_88E1149R		0x01410e50
#define MARVELL_PHY_ID_88E1240		0x01410e30
#define MARVELL_PHY_ID_88E1318S		0x01410e90
#define MARVELL_PHY_ID_88E1116R		0x01410e40
#define MARVELL_PHY_ID_88E1510		0x01410dd0
#define MARVELL_PHY_ID_88E1540		0x01410eb0
#define MARVELL_PHY_ID_88E1543		0x01410ea0

/* Mask used for ID comparisons */
#define MARVELL_PHY_ID_MASK		0xfffffff0

#endif /* _MARVELL_PHY_H */
