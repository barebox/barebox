/*
 *  (C) 2007, Sascha Hauer <sha@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <common.h>
#include <asm/arch/imx-regs.h>

void imx_gpio_mode(unsigned long mode)
{
	unsigned long reg, shift, tmp;

	reg = IOMUXC_BASE + (mode & 0xfc);
	shift = (~mode & 0x3) * 8;

	tmp = __REG(reg);
	tmp &= ~(0xff << shift);
	tmp |= ((mode >> 8) & 0xff) << shift;
	__REG(reg) = tmp;
}
