/*
 * Copyright (C) 2000, 2004, 2005  MIPS Technologies, Inc.
 *	All rights reserved.
 *	Authors: Carsten Langgaard <carstenl@mips.com>
 *		 Maciej W. Rozycki <macro@mips.com>
 * Copyright (C) 2005 Ralf Baechle (ralf@linux-mips.org)
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 */
#ifndef _ASM_GT64120_H
#define _ASM_GT64120_H

#define GT_DEF_BASE		0x14000000

/*
 *  Register offset addresses
 */

/* CPU Address Decode.	*/
#define GT_PCI0IOLD_OFS		0x048
#define GT_PCI0IOHD_OFS		0x050
#define GT_PCI0M0LD_OFS		0x058
#define GT_PCI0M0HD_OFS		0x060
#define GT_ISD_OFS		0x068

#define GT_PCI0M1LD_OFS		0x080
#define GT_PCI0M1HD_OFS		0x088

#endif /* _ASM_GT64120_H */
