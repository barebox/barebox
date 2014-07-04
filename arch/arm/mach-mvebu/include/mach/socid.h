/*
 * Marvell MVEBU SoC Ids
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

#ifndef __MACH_MVEBU_SOCID_H
#define __MACH_MVEBU_SOCID_H

extern u16 soc_devid;
extern u16 soc_revid;

static inline u16 mvebu_get_soc_devid(void)
{
	return soc_devid;
}

static inline u16 mvebu_get_soc_revid(void)
{
	return soc_revid;
}

/* Orion */
#define DEVID_F5180		0x5180
#define  REVID_F5180N_B1	0x3
#define DEVID_F5181		0x5181
#define  REVID_F5181_B1		0x3
#define  REVID_F5181L		0x8
#define DEVID_F5182		0x5182
#define  REVID_F5182_A1		0x1
#define DEVID_F6183		0x6183
/* Kirkwood */
#define DEVID_F6180		0x6180
#define DEVID_F6190		0x6190
#define DEVID_F6192		0x6192
#define DEVID_F6280		0x6280
#define DEVID_F6281		0x6281
#define DEVID_F6282		0x1155
/* Kirkwood Duo */
#define DEVID_F6321		0x6321
#define DEVID_F6322		0x6322
#define DEVID_F6323		0x6323
/* Avanta */
#define DEVID_F6510		0x6510
#define DEVID_F6530		0x6530
#define DEVID_F6550		0x6550
#define DEVID_F6560		0x6560
/* Dove */
#define DEVID_AP510		0x0510
#define DEVID_F6781		0x6781
/* Discovery Duo */
#define DEVID_MV76100		0x7610
#define DEVID_MV78100		0x7810
#define DEVID_MV78200		0x7820
/* Armada 370 */
#define DEVID_F6707		0x6707
#define DEVID_F6710		0x6710
#define DEVID_F6711		0x6711
/* Armada XP */
#define DEVID_MV78130		0x7813
#define DEVID_MV78160		0x7816
#define DEVID_MV78230		0x7823
#define DEVID_MV78260		0x7826
#define DEVID_MV78460		0x7846
#define DEVID_MV78880		0x7888

#endif /* __MACH_MVEBU_SOCID_H */
