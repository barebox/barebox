/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 *
 */

#ifndef _ASM_X86_SEGMENT_H
#define _ASM_X86_SEGMENT_H

#include <linux/compiler.h>

/**
 * @file
 * @brief To be able to mark functions running in real _and_ flat mode
 */

/**
 * Section for every program code needed to bring barebox from real mode
 * to flat mode
 */
#define __bootcode	__section(.boot.text)

/**
 * Section for every data needed to bring barebox from real mode
 * to flat mode
 */
#define __bootdata	__section(.boot.data)

#endif /* _ASM_X86_SEGMENT_H */
