/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Register and bit definitions for the MCF548X and MCF547x
 *  PCI Arbiter Module (PCIARB)
 */
#ifndef __MCF548X_PCIARB_H__
#define __MCF548X_PCIARB_H__

/*
 *  PCI Arbiter Module (PCIARB)
 */

/* Register read/write macros */
#define MCF_PCIARB_PACR    (*(vuint32_t*)(&__MBAR[0x000C00]))
#define MCF_PCIARB_PASR    (*(vuint32_t*)(&__MBAR[0x000C04]))

/* Bit definitions and macros for MCF_PCIARB_PACR */
#define MCF_PCIARB_PACR_INTMPRI         (0x00000001)
#define MCF_PCIARB_PACR_EXTMPRI(x)      (((x)&0x0000001F)<<1)
#define MCF_PCIARB_PACR_INTMINTEN       (0x00010000)
#define MCF_PCIARB_PACR_EXTMINTEN(x)    (((x)&0x0000001F)<<17)
/* Not documented!
 * #define MCF_PCIARB_PACR_PKMD            (0x40000000)
 */
#define MCF_PCIARB_PACR_DS              (0x80000000)

/* Bit definitions and macros for MCF_PCIARB_PASR */
#define MCF_PCIARB_PASR_ITLMBK          (0x00010000)
#define MCF_PCIARB_PASR_EXTMBK(x)       (((x)&0x0000001F)<<17)

#endif /* __MCF548X_PCIARB_H__ */
