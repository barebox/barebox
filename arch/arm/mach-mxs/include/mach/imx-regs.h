/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _IMX_REGS_H
# define _IMX_REGS_H

/* Note: Some registers do not support this bit change feature! */
#define BIT_SET 0x04
#define BIT_CLR 0x08
#define BIT_TGL 0x0C

#if defined CONFIG_ARCH_IMX23
# include <mach/imx23-regs.h>
#endif

#if defined CONFIG_ARCH_IMX28
# include <mach/imx28-regs.h>
#endif

#endif /* _IMX_REGS_H */
