/*
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH,
 * Author: Stefan Christ <s.christ@phytec.de>
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

#ifndef __NAND_MXS_H
#define __NAND_MXS_H

/*
 * Functions are definied in drivers/mtd/nand/nand_mxs.c.  They are used to
 * calculate the ECC Strength, BadBlockMarkerByte and BadBlockMarkerStartBit
 * which are placed into the FCB structure. The i.MX6 ROM needs these
 * parameters to read the firmware from NAND.
 *
 * The parameters depends on the pagesize and oobsize of NAND chips and are
 * different for each combination. To avoid placing hardcoded values in the bbu
 * update handler code, the generic calculation from the driver code is used.
 */

int mxs_nand_get_geo(int *ecc_strength, int *bb_mark_bit_offset);

#endif /* __NAND_MXS_H */
