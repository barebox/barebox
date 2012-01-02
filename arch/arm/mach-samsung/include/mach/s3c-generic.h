/*
 * (C) Copyright 2009
 * Juergen Beisert, Pengutronix
 *
 * (C) Copyright 2001-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
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

uint32_t s3c24xx_get_mpllclk(void);
uint32_t s3c24xx_get_upllclk(void);
uint32_t s3c24xx_get_fclk(void);
uint32_t s3c24xx_get_hclk(void);
uint32_t s3c24xx_get_pclk(void);
uint32_t s3c24xx_get_uclk(void);
uint32_t s3c24x0_get_memory_size(void);
