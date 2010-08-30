/*
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

#include <common.h>
#include <asm/io.h>
#include <mach/imx-regs.h>
#include <mach/iim.h>
#include <mach/generic.h>

#include "gpio.h"

void *imx_gpio_base[] = {
	(void *)0x53fcc000,
	(void *)0x53fd0000,
	(void *)0x53fa4000,
};

int imx_gpio_count = ARRAY_SIZE(imx_gpio_base) * 32;

int imx_silicon_revision()
{
	uint32_t reg;
	reg = readl(IMX_IIM_BASE + IIM_SREV);
	reg += IMX35_CHIP_REVISION_1_0;

	return (reg & 0xFF);
}
