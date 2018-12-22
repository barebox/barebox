/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2000, 2004, 2005  MIPS Technologies, Inc.
 *	All rights reserved.
 *	Authors: Carsten Langgaard <carstenl@mips.com>
 *		 Maciej W. Rozycki <macro@mips.com>
 * Copyright (C) 2005 Ralf Baechle (ralf@linux-mips.org)
 */
#ifndef _ASM_GT64120_H
#define _ASM_GT64120_H

#define MSK(n)			((1 << (n)) - 1)

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

#define GT_PCI0IOREMAP_OFS	0x0f0
#define GT_PCI0M0REMAP_OFS	0x0f8
#define GT_PCI0M1REMAP_OFS	0x100

/* Interrupts.	*/
#define GT_INTRCAUSE_OFS	0xc18

/* PCI Internal.  */
#define GT_PCI0_CMD_OFS		0xc00
#define GT_PCI0_CFGADDR_OFS	0xcf8
#define GT_PCI0_CFGDATA_OFS	0xcfc

#define GT_PCI_DCRM_SHF		21
#define GT_PCI_LD_SHF		0
#define GT_PCI_LD_MSK		(MSK(15) << GT_PCI_LD_SHF)
#define GT_PCI_HD_SHF		0
#define GT_PCI_HD_MSK		(MSK(7) << GT_PCI_HD_SHF)
#define GT_PCI_REMAP_SHF	0
#define GT_PCI_REMAP_MSK	(MSK(11) << GT_PCI_REMAP_SHF)

#define GT_INTRCAUSE_MASABORT0_SHF	18
#define GT_INTRCAUSE_MASABORT0_MSK	(MSK(1) << GT_INTRCAUSE_MASABORT0_SHF)
#define GT_INTRCAUSE_MASABORT0_BIT	GT_INTRCAUSE_MASABORT0_MSK

#define GT_INTRCAUSE_TARABORT0_SHF	19
#define GT_INTRCAUSE_TARABORT0_MSK	(MSK(1) << GT_INTRCAUSE_TARABORT0_SHF)
#define GT_INTRCAUSE_TARABORT0_BIT	GT_INTRCAUSE_TARABORT0_MSK

#define GT_PCI0_CFGADDR_REGNUM_SHF	2
#define GT_PCI0_CFGADDR_REGNUM_MSK	(MSK(6) << GT_PCI0_CFGADDR_REGNUM_SHF)
#define GT_PCI0_CFGADDR_FUNCTNUM_SHF	8
#define GT_PCI0_CFGADDR_FUNCTNUM_MSK	(MSK(3) << GT_PCI0_CFGADDR_FUNCTNUM_SHF)
#define GT_PCI0_CFGADDR_DEVNUM_SHF	11
#define GT_PCI0_CFGADDR_DEVNUM_MSK	(MSK(5) << GT_PCI0_CFGADDR_DEVNUM_SHF)
#define GT_PCI0_CFGADDR_BUSNUM_SHF	16
#define GT_PCI0_CFGADDR_BUSNUM_MSK	(MSK(8) << GT_PCI0_CFGADDR_BUSNUM_SHF)
#define GT_PCI0_CFGADDR_CONFIGEN_SHF	31
#define GT_PCI0_CFGADDR_CONFIGEN_MSK	(MSK(1) << GT_PCI0_CFGADDR_CONFIGEN_SHF)
#define GT_PCI0_CFGADDR_CONFIGEN_BIT	GT_PCI0_CFGADDR_CONFIGEN_MSK

#define GT_PCI0_CMD_MBYTESWAP_SHF       0
#define GT_PCI0_CMD_MBYTESWAP_MSK       (MSK(1) << GT_PCI0_CMD_MBYTESWAP_SHF)
#define GT_PCI0_CMD_MBYTESWAP_BIT       GT_PCI0_CMD_MBYTESWAP_MSK
#define GT_PCI0_CMD_SBYTESWAP_SHF       16
#define GT_PCI0_CMD_SBYTESWAP_MSK       (MSK(1) << GT_PCI0_CMD_SBYTESWAP_SHF)
#define GT_PCI0_CMD_SBYTESWAP_BIT       GT_PCI0_CMD_SBYTESWAP_MSK

/*
 * Because of an error/peculiarity in the Galileo chip, we need to swap the
 * bytes when running bigendian.  We also provide non-swapping versions.
 */
#define __GT_READ(ofs)							\
	(*(volatile u32 *)(GT64120_BASE+(ofs)))
#define __GT_WRITE(ofs, data)						\
	do { *(volatile u32 *)(GT64120_BASE+(ofs)) = (data); } while (0)
#define GT_READ(ofs)		le32_to_cpu(__GT_READ(ofs))
#define GT_WRITE(ofs, data)	__GT_WRITE(ofs, cpu_to_le32(data))

#endif /* _ASM_GT64120_H */
