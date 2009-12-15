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
 *  32KByte System SRAM (SRAM)
 */
#ifndef __MCF548X_SRAM_H__
#define __MCF548X_SRAM_H__

/*
 *  32KByte System SRAM (SRAM)
 */

/* Register read/write macros */
#define MCF_SRAM_SSCR       (*(vuint32_t*)(&__MBAR[0x01FFC0]))
#define MCF_SRAM_TCCR       (*(vuint32_t*)(&__MBAR[0x01FFC4]))
#define MCF_SRAM_TCCRDR     (*(vuint32_t*)(&__MBAR[0x01FFC8]))
#define MCF_SRAM_TCCRDW     (*(vuint32_t*)(&__MBAR[0x01FFCC]))
#define MCF_SRAM_TCCRSEC    (*(vuint32_t*)(&__MBAR[0x01FFD0]))

/* Bit definitions and macros for MCF_SRAM_SSCR */
#define MCF_SRAM_SSCR_INLV              (0x00010000)

/* Bit definitions and macros for MCF_SRAM_TCCR */
#define MCF_SRAM_TCCR_BANK0_TC(x)       (((x)&0x0000000F)<<0)
#define MCF_SRAM_TCCR_BANK1_TC(x)       (((x)&0x0000000F)<<8)
#define MCF_SRAM_TCCR_BANK2_TC(x)       (((x)&0x0000000F)<<16)
#define MCF_SRAM_TCCR_BANK3_TC(x)       (((x)&0x0000000F)<<24)

/* Bit definitions and macros for MCF_SRAM_TCCRDR */
#define MCF_SRAM_TCCRDR_BANK0_TC(x)     (((x)&0x0000000F)<<0)
#define MCF_SRAM_TCCRDR_BANK1_TC(x)     (((x)&0x0000000F)<<8)
#define MCF_SRAM_TCCRDR_BANK2_TC(x)     (((x)&0x0000000F)<<16)
#define MCF_SRAM_TCCRDR_BANK3_TC(x)     (((x)&0x0000000F)<<24)

/* Bit definitions and macros for MCF_SRAM_TCCRDW */
#define MCF_SRAM_TCCRDW_BANK0_TC(x)     (((x)&0x0000000F)<<0)
#define MCF_SRAM_TCCRDW_BANK1_TC(x)     (((x)&0x0000000F)<<8)
#define MCF_SRAM_TCCRDW_BANK2_TC(x)     (((x)&0x0000000F)<<16)
#define MCF_SRAM_TCCRDW_BANK3_TC(x)     (((x)&0x0000000F)<<24)

/* Bit definitions and macros for MCF_SRAM_TCCRSEC */
#define MCF_SRAM_TCCRSEC_BANK0_TC(x)    (((x)&0x0000000F)<<0)
#define MCF_SRAM_TCCRSEC_BANK1_TC(x)    (((x)&0x0000000F)<<8)
#define MCF_SRAM_TCCRSEC_BANK2_TC(x)    (((x)&0x0000000F)<<16)
#define MCF_SRAM_TCCRSEC_BANK3_TC(x)    (((x)&0x0000000F)<<24)

#endif /* __MCF548X_SRAM_H__ */
