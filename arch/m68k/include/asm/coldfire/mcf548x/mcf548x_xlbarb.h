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
 *  XLB bus arbiter
 */
#ifndef __MCF548X_XLBARB_H__
#define __MCF548X_XLBARB_H__

/*
 *  XLB arbiter register
 */
#define MCF_XLBARB_ACFG             (*(vuint32*)(&__MBAR[0x000240]))
#define MCF_XLBARB_VER              (*(vuint32*)(&__MBAR[0x000244]))
#define MCF_XLBARB_STA              (*(vuint32*)(&__MBAR[0x000248]))
#define MCF_XLBARB_INTEN            (*(vuint32*)(&__MBAR[0x00024C]))
#define MCF_XLBARB_ADRCAP           (*(vuint32*)(&__MBAR[0x000250]))
#define MCF_XLBARB_SIGCAP           (*(vuint32*)(&__MBAR[0x000254]))
#define MCF_XLBARB_ADRTO            (*(vuint32*)(&__MBAR[0x000258]))
#define MCF_XLBARB_DATTO            (*(vuint32*)(&__MBAR[0x00025C]))
#define MCF_XLBARB_BUSTO            (*(vuint32*)(&__MBAR[0x000260]))
#define MCF_XLBARB_PRIEN            (*(vuint32*)(&__MBAR[0x000264]))
#define MCF_XLBARB_PRI              (*(vuint32*)(&__MBAR[0x000268]))
#define MCF_XLBARB_BAR              (*(vuint32*)(&__MBAR[0x00026C]))


#endif /* __MCF548X_XLBARB_H__ */
