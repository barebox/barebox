/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of U-Boot V2.
 *
 * U-Boot V2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * U-Boot V2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with U-Boot V2.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  This File contains declaration for early output support
 */
#ifndef __INCLUDE_ARCH_DEBUG_LL_H__
#define   __INCLUDE_ARCH_DEBUG_LL_H__

extern __inline__ void putc( char ch )
{
	//extern int early_console_putc( char ch);
	early_console_putc(NULL,ch);
}

#endif  /* __INCLUDE_ARCH_DEBUG_LL_H__ */
