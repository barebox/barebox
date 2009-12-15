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
 *  Multi-Channel DMA External Requests (DMA_EREQ)
 */
#ifndef __MCF548X_DMA_EREQ_H__
#define __MCF548X_DMA_EREQ_H__

/*
 *  Multi-Channel DMA External Requests (DMA_EREQ)
 */

/* Register read/write macros */
#define MCF_DMA_EREQ_EREQBAR0       (*(vuint32_t*)(&__MBAR[0x000D00]))
#define MCF_DMA_EREQ_EREQMASK0      (*(vuint32_t*)(&__MBAR[0x000D04]))
#define MCF_DMA_EREQ_EREQCTRL0      (*(vuint32_t*)(&__MBAR[0x000D08]))
#define MCF_DMA_EREQ_EREQBAR1       (*(vuint32_t*)(&__MBAR[0x000D10]))
#define MCF_DMA_EREQ_EREQMASK1      (*(vuint32_t*)(&__MBAR[0x000D14]))
#define MCF_DMA_EREQ_EREQCTRL1      (*(vuint32_t*)(&__MBAR[0x000D18]))
#define MCF_DMA_EREQ_EREQBAR(x)     (*(vuint32_t*)(&__MBAR[0x000D00+((x)*0x010)]))
#define MCF_DMA_EREQ_EREQMASK(x)    (*(vuint32_t*)(&__MBAR[0x000D04+((x)*0x010)]))
#define MCF_DMA_EREQ_EREQCTRL(x)    (*(vuint32_t*)(&__MBAR[0x000D08+((x)*0x010)]))

/* Bit definitions and macros for MCF_DMA_EREQ_EREQCTRL */
#define MCF_DMA_EREQ_EREQCTRL_EN                   (0x00000001)
#define MCF_DMA_EREQ_EREQCTRL_SYNC                 (0x00000002)
#define MCF_DMA_EREQ_EREQCTRL_DACKWID(x)           (((x)&0x00000003)<<2)
#define MCF_DMA_EREQ_EREQCTRL_BSEL(x)              (((x)&0x00000003)<<4)
#define MCF_DMA_EREQ_EREQCTRL_MD(x)                (((x)&0x00000003)<<6)
#define MCF_DMA_EREQ_EREQCTRL_MD_IDLE              (0x00000000)
#define MCF_DMA_EREQ_EREQCTRL_MD_LEVEL             (0x00000040)
#define MCF_DMA_EREQ_EREQCTRL_MD_EDGE              (0x00000080)
#define MCF_DMA_EREQ_EREQCTRL_MD_PIPED             (0x000000C0)
#define MCF_DMA_EREQ_EREQCTRL_BSEL_MEM_WRITE       (0x00000000)
#define MCF_DMA_EREQ_EREQCTRL_BSEL_MEM_READ        (0x00000010)
#define MCF_DMA_EREQ_EREQCTRL_BSEL_PERIPH_WRITE    (0x00000020)
#define MCF_DMA_EREQ_EREQCTRL_BSEL_PERIPH_READ     (0x00000030)
#define MCF_DMA_EREQ_EREQCTRL_DACKWID_ONE          (0x00000000)
#define MCF_DMA_EREQ_EREQCTRL_DACKWID_TWO          (0x00000004)
#define MCF_DMA_EREQ_EREQCTRL_DACKWID_THREE        (0x00000008)
#define MCF_DMA_EREQ_EREQCTRL_DACKWID_FOUR         (0x0000000C)

#endif /* __MCF548X_DMA_EREQ_H__ */
