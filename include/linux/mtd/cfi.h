/*
 * Copyright © 2000-2010 David Woodhouse <dwmw2@infradead.org> et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __MTD_CFI_H__
#define __MTD_CFI_H__

/* NB: these values must represents the number of bytes needed to meet the
 *     device type (x8, x16, x32).  Eg. a 32 bit device is 4 x 8 bytes.
 *     These numbers are used in calculations.
 */
#define CFI_DEVICETYPE_X8  (8 / 8)
#define CFI_DEVICETYPE_X16 (16 / 8)
#define CFI_DEVICETYPE_X32 (32 / 8)
#define CFI_DEVICETYPE_X64 (64 / 8)


/* Device Interface Code Assignments from the "Common Flash Memory Interface
 * Publication 100" dated December 1, 2001.
 */
#define CFI_INTERFACE_X8_ASYNC		0x0000
#define CFI_INTERFACE_X16_ASYNC		0x0001
#define CFI_INTERFACE_X8_BY_X16_ASYNC	0x0002
#define CFI_INTERFACE_X32_ASYNC		0x0003
#define CFI_INTERFACE_X16_BY_X32_ASYNC	0x0005
#define CFI_INTERFACE_NOT_ALLOWED	0xffff


#define CFI_MFR_ANY		0xFFFF
#define CFI_ID_ANY		0xFFFF
#define CFI_MFR_CONTINUATION	0x007F

#define CFI_MFR_AMD		0x0001
#define CFI_MFR_AMIC		0x0037
#define CFI_MFR_ATMEL		0x001F
#define CFI_MFR_EON		0x001C
#define CFI_MFR_FUJITSU		0x0004
#define CFI_MFR_HYUNDAI		0x00AD
#define CFI_MFR_INTEL		0x0089
#define CFI_MFR_MACRONIX	0x00C2
#define CFI_MFR_NEC		0x0010
#define CFI_MFR_PMC		0x009D
#define CFI_MFR_SAMSUNG		0x00EC
#define CFI_MFR_SHARP		0x00B0
#define CFI_MFR_SST		0x00BF
#define CFI_MFR_ST		0x0020 /* STMicroelectronics */
#define CFI_MFR_TOSHIBA		0x0098
#define CFI_MFR_WINBOND		0x00DA

#endif /* __MTD_CFI_H__ */
