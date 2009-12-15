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
 *  Slice Timers (SLT)
 */
#ifndef __MCF548X_SLT_H__
#define __MCF548X_SLT_H__

/*
 * Slice Timers (SLT)
 */

/* Register read/write macros */
#define MCF_SLT_SLTCNT0      (*(vuint32_t*)(&__MBAR[0x000900]))
#define MCF_SLT_SCR0         (*(vuint32_t*)(&__MBAR[0x000904]))
#define MCF_SLT_SCNT0        (*(vuint32_t*)(&__MBAR[0x000908]))
#define MCF_SLT_SSR0         (*(vuint32_t*)(&__MBAR[0x00090C]))

#define MCF_SLT_SLTCNT1      (*(vuint32_t*)(&__MBAR[0x000910]))
#define MCF_SLT_SCR1         (*(vuint32_t*)(&__MBAR[0x000914]))
#define MCF_SLT_SCNT1        (*(vuint32_t*)(&__MBAR[0x000918]))
#define MCF_SLT_SSR1         (*(vuint32_t*)(&__MBAR[0x00091C]))

#define MCF_SLT_SLTCNT(x)    (*(vuint32_t*)(&__MBAR[0x000900+((x)*0x010)]))
#define MCF_SLT_SCR(x)       (*(vuint32_t*)(&__MBAR[0x000904+((x)*0x010)]))
#define MCF_SLT_SCNT(x)      (*(vuint32_t*)(&__MBAR[0x000908+((x)*0x010)]))
#define MCF_SLT_SSR(x)       (*(vuint32_t*)(&__MBAR[0x00090C+((x)*0x010)]))

/* Bit definitions and macros for MCF_SLT_SCR */
#define MCF_SLT_SCR_TEN    (0x01000000)
#define MCF_SLT_SCR_IEN    (0x02000000)
#define MCF_SLT_SCR_RUN    (0x04000000)

/* Bit definitions and macros for MCF_SLT_SSR */
#define MCF_SLT_SSR_ST     (0x01000000)
#define MCF_SLT_SSR_BE     (0x02000000)


#ifndef __ASSEMBLY__

#define MCF_SLT_Address(x)    ((struct mcf5xxx_slt*)(void*)(&__MBAR[0x000900+((x)*0x010)]))

struct mcf5xxx_slt {
	vuint32_t STCNT;		/* Slice Terminal Count */
	vuint32_t SCR;            /* Slice Timer Control Register */
	vuint32_t SCNT;           /* Slice Count Value */
	vuint32_t SSR;		/* Slice Timer Status Register */
};

#endif

#endif /* __MCF548X_SLT_H__ */
