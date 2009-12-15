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
 *  DMA Serial Peripheral Interface (DSPI)
 */
#ifndef __MCF548X_SIU_H__
#define __MCF548X_SIU_H__

/*
 * System Integration Unit (SIU)
 */

/* Register read/write macros */
#define MCF_SIU_SBCR       (*(vuint32_t*)(&__MBAR[0x000010]))
#define MCF_SIU_SECSACR    (*(vuint32_t*)(&__MBAR[0x000038]))
#define MCF_SIU_RSR        (*(vuint32_t*)(&__MBAR[0x000044]))
#define MCF_SIU_JTAGID     (*(vuint32_t*)(&__MBAR[0x000050]))

/* Bit definitions and macros for MCF_SIU_SBCR */
#define MCF_SIU_SBCR_PIN2DSPI       (0x08000000)
#define MCF_SIU_SBCR_DMA2CPU        (0x10000000)
#define MCF_SIU_SBCR_CPU2DMA        (0x20000000)
#define MCF_SIU_SBCR_PIN2DMA        (0x40000000)
#define MCF_SIU_SBCR_PIN2CPU        (0x80000000)

/* Bit definitions and macros for MCF_SIU_SECSACR */
#define MCF_SIU_SECSACR_SEQEN       (0x00000001)

/* Bit definitions and macros for MCF_SIU_RSR */
#define MCF_SIU_RSR_RST             (0x00000001)
#define MCF_SIU_RSR_RSTWD           (0x00000002)
#define MCF_SIU_RSR_RSTJTG          (0x00000008)

/* Bit definitions and macros for MCF_SIU_JTAGID */
#define MCF_SIU_JTAGID_REV          (0xF0000000)
#define MCF_SIU_JTAGID_PROCESSOR    (0x0FFFFFFF)
#define MCF_SIU_JTAGID_MCF5485      (0x0800C01D)
#define MCF_SIU_JTAGID_MCF5484      (0x0800D01D)
#define MCF_SIU_JTAGID_MCF5483      (0x0800E01D)
#define MCF_SIU_JTAGID_MCF5482      (0x0800F01D)
#define MCF_SIU_JTAGID_MCF5481      (0x0801001D)
#define MCF_SIU_JTAGID_MCF5480      (0x0801101D)
#define MCF_SIU_JTAGID_MCF5475      (0x0801201D)
#define MCF_SIU_JTAGID_MCF5474      (0x0801301D)
#define MCF_SIU_JTAGID_MCF5473      (0x0801401D)
#define MCF_SIU_JTAGID_MCF5472      (0x0801501D)
#define MCF_SIU_JTAGID_MCF5471      (0x0801601D)
#define MCF_SIU_JTAGID_MCF5470      (0x0801701D)

#endif /* __MCF548X_SIU_H__ */
