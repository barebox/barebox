/**
 * @file
 * @brief Global defintions for the ARM i.MX27 based pcm038
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* FIXME: ugly....should be simply part of the BSP file */

#include <asm/mach-types.h>

#define CONFIG_BOOT_PARAMS	0xa0000100
#define CFG_MALLOC_LEN		(4096 << 10)
#define CONFIG_STACKSIZE	( 120 << 10)	/* stack size */

#endif	/* __CONFIG_H */
